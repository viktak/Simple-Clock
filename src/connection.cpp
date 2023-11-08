#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "connection.h"
#include "main.h"
#include "ntp.h"
#include "mqtt.h"
#include "leds.h"
#include "daisy595.h"
#include "asyncwebserver.h"

#define MAX_WIFI_INACTIVITY 300
#define WIFI_CONNECTION_TIMEOUT 10
#define ACCESS_POINT_TIMEOUT 300000

bool ntpInitialized = false;
bool isAccessPoint = false;
bool isAccessPointCreated = false;
bool needsRestart = false;

unsigned long timeMillis = 0;
int oldMin = 99;
int oldSec = 99;

const char *ntpServerName = "diy.viktak.com";

os_timer_t accessPointTimer;

void accessPointTimerCallback(void *pArg)
{
    ESP.reset();
}

enum CONNECTION_STATE
{
    STATE_CHECK_WIFI_CONNECTION,
    STATE_WIFI_CONNECT,
    STATE_CHECK_INTERNET_CONNECTION,
    STATE_INTERNET_CONNECTED
} connectionState;

boolean checkInternetConnection()
{
    IPAddress timeServerIP;
    int result = WiFi.hostByName(ntpServerName, timeServerIP);
    return (result == 1);
}

void loopConnection()
{
    if (isAccessPoint)
    {
        if (!isAccessPointCreated)
        {
            Serial.printf("Could not connect to %s.\r\nReverting to Access Point mode.\r\n", appSettings.wifiSSID);

            delay(500);
            WiFi.mode(WIFI_AP);
            WiFi.softAP(appSettings.localHost, appSettings.accessPointPassword);

            IPAddress myIP = WiFi.softAPIP();
            isAccessPointCreated = true;

            // InitWifiWebServer();
            InitAsyncWebServer();

            Serial.println("Access point created. Use the following information to connect to the ESP device, then follow the on-screen instructions to connect to a different wifi network:");

            Serial.print("SSID:\t\t\t");
            Serial.println(appSettings.localHost);

            Serial.print("Password:\t\t");
            Serial.println(appSettings.accessPointPassword);

            Serial.print("Access point address:\t");
            Serial.println(myIP);

            //  Timers/interrupts
            os_timer_setfn(&accessPointTimer, accessPointTimerCallback, NULL);
            os_timer_arm(&accessPointTimer, ACCESS_POINT_TIMEOUT, true);
            os_timer_disarm(&heartbeatTimer);
        }
    }
    else
    {
        switch (connectionState)
        {

        // Check the WiFi connection
        case STATE_CHECK_WIFI_CONNECTION:
        {
            // Are we connected ?
            if (WiFi.status() != WL_CONNECTED)
            {
                // Wifi is NOT connected
                connectionLED_OFF();
                connectionState = STATE_WIFI_CONNECT;
            }
            else
            {
                // Wifi is connected so check Internet
                connectionLED_ON();
                connectionState = STATE_CHECK_INTERNET_CONNECTION;
            }
            break;
        }

        // No Wifi so attempt WiFi connection
        case STATE_WIFI_CONNECT:
        {
            // Indicate NTP no yet initialized
            ntpInitialized = false;
            connectionLED_OFF();
            Serial.printf("Trying to connect to WIFI network: %s", appSettings.wifiSSID);

            // Set station mode
            WiFi.mode(WIFI_STA);

            // Start connection process
            WiFi.hostname(appSettings.localHost); //  so that it shows up coorectly in DHCP/DNS on the router
            WiFi.begin(appSettings.wifiSSID, appSettings.wifiPassword);

            // Initialize iteration counter
            uint8_t attempt = 0;

            while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT))
            {
                disp.DoWifiAnimation(attempt);

                connectionLED_ON();
                Serial.print(".");
                delay(50);
                connectionLED_OFF();
                delay(950);
            }
            disp.ClearDisplay();

            if (attempt >= WIFI_CONNECTION_TIMEOUT)
            {
                Serial.println();
                Serial.println("Could not connect to WiFi.");
                delay(100);

                isAccessPoint = true;
                isAccessPointCreated = false;

                break;
            }
            connectionLED_ON();

            Serial.println(" Success!");

            IPAddress ip = WiFi.localIP();

            Serial.printf("WiFi channel:\t%u\r\n", WiFi.channel());
            Serial.printf("IP address:\t%s\r\n", ip.toString().c_str());
            connectionState = STATE_CHECK_INTERNET_CONNECTION;
            break;
        }

        case STATE_CHECK_INTERNET_CONNECTION:
        {
            // Do we have a connection to the Internet ?
            if (checkInternetConnection())
            {
                // We have an Internet connection
                if (!ntpInitialized)
                {
                    // We are connected to the Internet for the first time so set NTP provider
                    setupNTP();

                    ntpInitialized = true;

                    InitAsyncWebServer();

                    Serial.println("Connected to the Internet.");
                }

                connectionState = STATE_INTERNET_CONNECTED;
                connectionLED_OFF();
            }
            else
            {
                connectionState = STATE_CHECK_WIFI_CONNECTION;
                connectionLED_ON();
            }
            break;
        }

        case STATE_INTERNET_CONNECTED:
        {
            loopMQTT();
            loopNTP();
            loopAsyncWebserver();

            if (millis() - timeMillis > 1000)
            {
                time_t localTime = timechangerules::timezones[appSettings.timeZone]->toLocal(now(), &tcr);
                if (oldSec != second(localTime))
                {
                    disp.SetDigitsToHoursMinutes(localTime);
                    disp.RefreshDisplay();
                    oldSec = second(localTime);
                }
                timeMillis = millis();
            }

            // Set next connection state
            connectionState = STATE_CHECK_WIFI_CONNECTION;
            break;
        }
        }
    }
}

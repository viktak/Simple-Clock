#include <Arduino.h>

#include <DNSServer.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <AceSorting.h>

#include "asyncwebserver.h"
#include "common.h"
#include "version.h"
#include "settings.h"
#include "main.h"
#include "TimeChangeRules.h"

const char *ADMIN_USERNAME = "admin";
const char *ADMIN_PASSWORD = "admin";

AsyncWebServer server(80);

size_t updateSize = 0;
bool shouldReboot = false;
unsigned long ota_progress_millis = 0;
size_t lastCurrent = 0;

//  Helper functions - not used in production
void PrintParameters(AsyncWebServerRequest *request)
{
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isFile())
        { // p->isPost() is also true
            SerialMon.printf("FILE[%s]: %s, size: %u\r\n", p->name().c_str(), p->value().c_str(), p->size());
        }
        else if (p->isPost())
        {
            SerialMon.printf("POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        }
        else
        {
            SerialMon.printf("GET[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        }
    }
}

void PrintHeaders(AsyncWebServerRequest *request)
{
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++)
    {
        AsyncWebHeader *h = request->getHeader(i);
        SerialMon.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
}

// Root/Status
String StatusTemplateProcessor(const String &var)
{
    //  System information
    if (var == "chipid")
        return (String)ESP.getChipId();
    if (var == "hardwareid")
        return HARDWARE_ID;
    if (var == "hardwareversion")
        return HARDWARE_VERSION;
    if (var == "firmwareid")
        return FIRMWARE_ID;
    if (var == "firmwareversion")
        return FIRMWARE_VERSION;
    if (var == "timezone")
        return timechangerules::tzDescriptions[appSettings.timeZone];
    if (var == "currenttime")
    {
        TimeChangeRule *tcr;
        time_t localTime = timechangerules::timezones[appSettings.timeZone]->toLocal(now(), &tcr);
        return DateTimeToString(localTime);
    }
    if (var == "uptime")
        return TimeIntervalToString(millis() / 1000);
    if (var == "lastresetreason0")
        return ESP.getResetInfo();
    if (var == "flashchipsize")
        return String(ESP.getFlashChipSize());
    if (var == "flashchipspeed")
        return String(ESP.getFlashChipSpeed());
    if (var == "freeheapsize")
        return String(ESP.getFreeHeap());
    if (var == "freesketchspace")
        return String(ESP.getFreeSketchSpace());

    //  Network

    switch (WiFi.getMode())
    {
    case WIFI_AP:
        if (var == "wifimode")
            return "Access Point";
        if (var == "ssid")
            return "-";
        if (var == "channel")
            return String(WiFi.channel());
        if (var == "hostname")
            return appSettings.localHost;
        if (var == "macaddress")
            return "-";
        if (var == "ipaddress")
            return WiFi.softAPIP().toString();
        // if (var == "subnetmask")
        //     return WiFi.softAPSubnetMask().toString();
        if (var == "apssid")
            return appSettings.localHost;
        if (var == "apmacaddress")
            return String(WiFi.softAPmacAddress());
        if (var == "gateway")
            return "-";
        break;

    case WIFI_STA:
        if (var == "wifimode")
            return "Station";
        if (var == "ssid")
            return String(WiFi.SSID());
        if (var == "channel")
            return String(WiFi.channel());
        if (var == "hostname")
            return appSettings.localHost;
        if (var == "macaddress")
            return String(WiFi.macAddress());
        if (var == "ipaddress")
            return WiFi.localIP().toString();
        if (var == "subnetmask")
            return WiFi.subnetMask().toString();
        if (var == "apssid")
            return "Not active!";
        if (var == "apmacaddress")
            return "Not active!";
        if (var == "gateway")
            return WiFi.gatewayIP().toString();

        break;
    default:
        //  This should not happen...
        break;
    }

    return String();
}

//  General
String GeneralSettingsTemplateProcessor(const String &var)
{
    if (var == "friendlyname")
        return appSettings.nodeFriendlyName;

    if (var == "mqtt-servername")
        return appSettings.mqttServer;

    if (var == "mqtt-port")
        return (String)appSettings.mqttPort;

    if (var == "mqtt-topic")
        return appSettings.mqttTopic;

    if (var == "HeartbeatInterval")
        return (String)appSettings.heartbeatInterval;

    if (var == "timezoneslist")
    {
        char ss[4];
        String timezoneslist = "";

        for (unsigned int i = 0; i < sizeof(timechangerules::tzDescriptions) / sizeof(timechangerules::tzDescriptions[0]); i++)
        {
            itoa(i, ss, DEC);
            timezoneslist += "<option ";
            if ((unsigned int)appSettings.timeZone == i)
                timezoneslist += "selected ";

            timezoneslist += "value=\"";
            timezoneslist += ss;
            timezoneslist += "\">";

            timezoneslist += timechangerules::tzDescriptions[i];

            timezoneslist += "</option>";
        }

        return timezoneslist;
    }

    return String();
}

void POSTGeneralSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    PrintParameters(request);

    if (request->hasParam("txtFriendlyName", true))
    {
        p = request->getParam("txtFriendlyName", true);
        sprintf(appSettings.nodeFriendlyName, "%s", p->value().c_str());
    }

    if (request->hasParam("txtServerName", true))
    {
        p = request->getParam("txtServerName", true);
        sprintf(appSettings.mqttServer, "%s", p->value().c_str());
    }

    if (request->hasParam("numPort", true))
    {
        p = request->getParam("numPort", true);
        appSettings.mqttPort = atoi(p->value().c_str());
    }

    if (request->hasParam("txtTopic", true))
    {
        p = request->getParam("txtTopic", true);
        sprintf(appSettings.mqttTopic, "%s", p->value().c_str());
    }

    if (request->hasParam("lstTimeZones", true))
    {
        p = request->getParam("lstTimeZones", true);
        appSettings.timeZone = atoi(p->value().c_str());
    }

    if (request->hasParam("numHeartbeatInterval", true))
    {
        p = request->getParam("numHeartbeatInterval", true);
        appSettings.heartbeatInterval = atoi(p->value().c_str());
    }

    appSettings.Save();
}

//  Clock
String ClockSettingsTemplateProcessor(const String &var)
{
    if (var == "optDisplayHoursAndMinutesCheck")
        return appSettings.clockMode == 0 ? "checked" : "";

    if (var == "optDisplayMinutesAndSecondsCheck")
        return appSettings.clockMode == 1 ? "checked" : "";

    SerialMon.println(var);

    return String();
}

void POSTClockSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    PrintParameters(request);

    if (request->hasParam("optClockType", true))
    {
        p = request->getParam("optClockType", true);
        appSettings.clockMode = atoi(p->value().c_str());
    }
    appSettings.Save();
}

//  Network
String NetworkSettingsTemplateProcessor(const String &var)
{
    if (var == "networklist")
    {
        int numberOfNetworks = WiFi.scanComplete();
        if (numberOfNetworks == -2)
        {
            WiFi.scanNetworks(true);
        }
        else if (numberOfNetworks)
        {
            String aSSIDs[numberOfNetworks];
            for (int i = 0; i < numberOfNetworks; ++i)
            {
                aSSIDs[i] = WiFi.SSID(i);
            }

            ace_sorting::insertionSort(aSSIDs, numberOfNetworks);

            String wifiList;
            for (int i = 0; i < numberOfNetworks; i++)
            {
                wifiList += "<option value='";
                wifiList += aSSIDs[i];
                wifiList += "'";

                if (!strcmp(aSSIDs[i].c_str(), (appSettings.wifiSSID)))
                {
                    wifiList += " selected ";
                }

                wifiList += ">";
                wifiList += aSSIDs[i] + " (" + (String)WiFi.RSSI(i) + ")";
                wifiList += "</option>";
            }
            WiFi.scanDelete();
            if (WiFi.scanComplete() == -2)
            {
                WiFi.scanNetworks(true);
            }
            return wifiList;
        }
    }

    if (var == "password")
        return "";

    return String();
}

void POSTNetworkSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("lstNetworks", true))
    {
        p = request->getParam("lstNetworks", true);
        sprintf(appSettings.wifiSSID, "%s", p->value().c_str());
    }

    if (request->hasParam("txtPassword", true))
    {
        p = request->getParam("txtPassword", true);
        sprintf(appSettings.wifiPassword, "%s", p->value().c_str());
    }

    appSettings.Save();
}

//  Tools
String ToolsTemplateProcessor(const String &var)
{
    return String();
}

//  OTA
void onOTAStart()
{
    Serial.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final)
{
    if (millis() - ota_progress_millis > 500)
    {
        Serial.printf("OTA: %3.0f%%\tTransfered %7u of %7u bytes\tSpeed: %4.3fkB/s\r", ((float)current / (float) final) * 100, current, final, ((float)(current - lastCurrent)) / 1024 / 10);
        lastCurrent = current;
        ota_progress_millis = millis();
    }
}

void onOTAEnd(bool success)
{
    Serial.println("\rOTA: 100%\r\n"); //  Overwrite the progress value

    if (success)
    {
        Serial.println("OTA update finished successfully! Restarting...");
    }
    else
    {
        Serial.println("\r\nThere was an error during OTA update!");
    }
}

/// Init
void InitAsyncWebServer()
{
    //  Bootstrap
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/bootstrap.bundle.min.js", "text/javascript"); });

    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/bootstrap.min.css", "text/css"); });

    //  Images
    server.on("/favico.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favico.ico", "image/x-icon"); });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favico.icon", "image/x-icon"); });

    server.on("/menu.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/menu.png", "image/png"); });

    //  Logout
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  request->requestAuthentication();
                  request->redirect("/"); });

    //  Status
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/status.html", String(), false, StatusTemplateProcessor); });

    server.on("/status.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/status.html", String(), false, StatusTemplateProcessor); });

    //  General
    server.on("/general-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD))
                    return request->requestAuthentication();
                request->send(LittleFS, "/general-settings.html", String(), false, GeneralSettingsTemplateProcessor); });

    server.on("/general-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  POSTGeneralSettings(request);

                  ESP.restart();

                  request->redirect("/general-settings.html"); });

    //  Clock
    server.on("/clock-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD))
                    return request->requestAuthentication();
                request->send(LittleFS, "/clock-settings.html", String(), false, ClockSettingsTemplateProcessor); });

    server.on("/clock-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  POSTClockSettings(request);
                  request->redirect("/clock-settings.html"); });

    //  Network
    server.on("/network-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD))
                    return request->requestAuthentication();
                
                request->send(LittleFS, "/network-settings.html", String(), false, NetworkSettingsTemplateProcessor); });

    server.on("/network-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  PrintParameters(request);
                  POSTNetworkSettings(request);

                  ESP.restart();

                  request->redirect("/network-settings.html"); });

    //  Tools
    server.on("/tools.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD))
                        return request->requestAuthentication();
                    request->send(LittleFS, "/tools.html", String(), false, ToolsTemplateProcessor); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD))
                        return request->requestAuthentication();
                    ESP.restart(); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, ADMIN_PASSWORD))
                        return request->requestAuthentication();
                    appSettings.LoadDefaults();
                    ESP.restart(); });

    //  OTA
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server, ADMIN_USERNAME, ADMIN_PASSWORD);
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();
    SerialMon.println("AsyncWebServer started.");
}

void loopAsyncWebserver()
{
    ElegantOTA.loop();
}
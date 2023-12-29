#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

#include "mqtt.h"
#include "main.h"
#include "version.h"
#include "common.h"

bool needsHeartbeat = false;
os_timer_t heartbeatTimer;

WiFiClient client;

PubSubClient PSclient(client);

const char *mqttCustomer = MQTT_CUSTOMER;
const char *mqttProject = MQTT_PROJECT;

void heartbeatTimerCallback(void *pArg)
{
    needsHeartbeat = true;
}

void ConnectToMQTTBroker()
{
    if (!PSclient.connected())
    {
#ifdef __debugSettings
        Serial.printf("Connecting to MQTT broker %s:%u with ClientID %s... ", appSettings.mqttServer, appSettings.mqttPort, appSettings.localHost);
#endif
        if (PSclient.connect(appSettings.localHost, (mqttCustomer + String("/") + mqttProject + String("/") + appSettings.mqttTopic + "/STATE").c_str(), 0, true, "offline"))
        {
#ifdef __debugSettings
            Serial.println(" success.");
#endif
            PSclient.subscribe((mqttCustomer + String("/") + mqttProject + String("/") + appSettings.mqttTopic + "/cmnd").c_str(), 0);
            PSclient.publish((mqttCustomer + String("/") + mqttProject + String("/") + appSettings.mqttTopic + "/STATE").c_str(), "online", true);

            PSclient.setBufferSize(1024 * 5);
        }
        else
        {
#ifdef __debugSettings
            Serial.println(" failure!");
#endif
        }
    }
}

void PublishData(const char *topic, const char *payload, bool retained)
{
    ConnectToMQTTBroker();

    if (PSclient.connected())
    {
        PSclient.publish((mqttCustomer + String("/") + mqttProject + String("/") + appSettings.mqttTopic + String("/") + topic).c_str(), payload, retained);
    }
}

void SendHeartbeat()
{
    StaticJsonDocument<768> doc;

    JsonObject sysDetails = doc.createNestedObject("System");
    sysDetails["ChipID"] = (String)ESP.getChipId();

    TimeChangeRule *tcr;
    time_t localTime = timechangerules::timezones[appSettings.timeZone]->toLocal(now(), &tcr);
    sysDetails["Time"] = DateTimeToString(localTime);

    sysDetails["Node"] = appSettings.localHost;
    sysDetails["Freeheap"] = ESP.getFreeHeap();
    sysDetails["clockMode"] = appSettings.clockMode;

    sysDetails["HardwareID"] = HARDWARE_ID;
    sysDetails["HardwareVersion"] = HARDWARE_VERSION;
    sysDetails["FirmwareID"] = FIRMWARE_ID;
    sysDetails["FirmwareVersion"] = FIRMWARE_VERSION;
    sysDetails["UpTime"] = TimeIntervalToString(millis() / 1000);
    sysDetails["CPU0_ResetReason"] = ESP.getResetReason();

    sysDetails["FriendlyName"] = appSettings.nodeFriendlyName;
    sysDetails["TIMEZONE"] = appSettings.timeZone;

    JsonObject mqttDetails = doc.createNestedObject("MQTT");

    mqttDetails["MQTT_SERVER"] = appSettings.mqttServer;
    mqttDetails["MQTT_PORT"] = appSettings.mqttPort;
    mqttDetails["MQTT_TOPIC"] = appSettings.mqttTopic;

    JsonObject wifiDetails = doc.createNestedObject("WiFi");
    wifiDetails["AP_NAME"] = appSettings.localHost;
    wifiDetails["SSID"] = appSettings.wifiSSID;
    wifiDetails["Channel"] = WiFi.channel();
    wifiDetails["IP_Address"] = WiFi.localIP().toString();
    wifiDetails["MAC_Address"] = WiFi.macAddress();

    String myJsonString;

    serializeJson(doc, myJsonString);

#ifdef __debugSettings
    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    ConnectToMQTTBroker();

    if (PSclient.connected())
    {
        PSclient.publish((mqttCustomer + String("/") + mqttProject + String("/") + appSettings.mqttTopic + "/HEARTBEAT").c_str(), myJsonString.c_str(), false);
#ifdef __debugSettings
        Serial.println("Heartbeat sent.");
#endif
        needsHeartbeat = false;
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
    StaticJsonDocument<32> doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (error)
        return;

#ifdef __debugSettings
    Serial.print("Message arrived in topic [");
    Serial.print(topic);
    Serial.println("]: ");

    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    String command = doc["command"];
    command.toUpperCase();
}

void setupMQTT()
{
    PSclient.setServer(appSettings.mqttServer, appSettings.mqttPort);
    PSclient.setCallback(mqttCallback);

    os_timer_setfn(&heartbeatTimer, heartbeatTimerCallback, NULL);
    os_timer_arm(&heartbeatTimer, appSettings.heartbeatInterval * 1000, true);
}

void loopMQTT()
{
    ConnectToMQTTBroker();
    if (PSclient.connected())
        PSclient.loop();

    if (needsHeartbeat)
        SendHeartbeat();
}

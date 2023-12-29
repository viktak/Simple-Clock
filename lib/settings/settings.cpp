#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "ArduinoJson.h"

#include "settings.h"

#define FORMAT_SPIFFS_IF_FAILED true

String settings::GetDeviceMAC()
{
    String s = WiFi.macAddress();

    for (size_t i = 0; i < 5; i++)
        s.remove(14 - i * 3, 1);

    return s;
}

settings::settings()
{
    if (!LittleFS.begin())
        Serial.println("Error: Failed to initialize the filesystem!");
}

bool settings::Load()
{

    sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        Serial.println("Failed to open config file.");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1024)
    {
        Serial.println("Config file size is too large.");
        return false;
    }

    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // We don't use String here because ArduinoJson library requires the input
    // buffer to be mutable. If you don't use ArduinoJson, you may as well
    // use configFile.readString instead.
    configFile.readBytes(buf.get(), size);
    configFile.close();

    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, buf.get());

    if (error)
    {
        Serial.println("Failed to parse config file.");
        Serial.println(error.c_str());
        return false;
    }

#ifdef __debugSettings
    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    if (doc["ssid"])
    {
        strcpy(wifiSSID, doc["ssid"]);
    }
    else
    {
        sprintf(wifiSSID, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());
    }

    if (doc["password"])
    {
        strcpy(wifiPassword, doc["password"]);
    }

    if (doc["accessPointPassword"])
    {
        strcpy(accessPointPassword, doc["accessPointPassword"]);
    }

    if (doc["mqttServer"])
    {
        strcpy(mqttServer, doc["mqttServer"]);
    }

    if (doc["mqttPort"])
    {
        mqttPort = doc["mqttPort"];
    }

    if (doc["mqttTopic"])
    {
        strcpy(mqttTopic, doc["mqttTopic"]);
    }
    else
    {
        sprintf(mqttTopic, localHost);
    }

    if (doc["friendlyName"])
    {
        strcpy(nodeFriendlyName, doc["friendlyName"]);
    }

    if (doc["timezone"])
    {
        timeZone = doc["timezone"];
    }

    if (doc["heartbeatInterval"])
    {
        heartbeatInterval = doc["heartbeatInterval"];
    }

    if (doc["clockMode"])
    {
        clockMode = doc["clockMode"];
    }

    if (strcmp(localHost, mqttTopic) != 0)
    {
        char mac[7];
        strcpy(mac, GetDeviceMAC().substring(6).c_str());

        char topic[sizeof(localHost) + sizeof(mac) + 1];
        for (size_t i = 0; i < sizeof(topic); i++)
        {
            topic[i] = 0;
        }

        size_t pos = 0;
        for (size_t i = 0; i < 25; i++)
        {
            if (mqttTopic[i] != 0)
            {
                topic[i] = mqttTopic[i];
                pos = i;
            }
            else
            {
                pos++;
                break;
            }
        }

        topic[pos++] = '-';

        for (size_t i = 0; i < sizeof(mac); i++)
        {
            topic[pos + i] = mac[i];
        }

        strcpy(localHost, topic);
    }

    return true;
}

bool settings::Save()
{
    StaticJsonDocument<768> doc;

    doc["ssid"] = wifiSSID;
    doc["password"] = wifiPassword;
    doc["accessPointPassword"] = accessPointPassword;

    doc["heartbeatInterval"] = heartbeatInterval;

    doc["timezone"] = timeZone;

    doc["mqttServer"] = mqttServer;
    doc["mqttPort"] = mqttPort;
    doc["mqttTopic"] = mqttTopic;

    doc["friendlyName"] = nodeFriendlyName;
    doc["clockMode"] = clockMode;

#ifdef __debugSettings
    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    serializeJson(doc, configFile);
    configFile.close();

    return true;
}

void settings::LoadDefaults()
{
    sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());

    strcpy(wifiSSID, localHost);
    strcpy(wifiPassword, DEFAULT_AP_PASSWORD);

    strcpy(mqttServer, DEFAULT_MQTT_SERVER);
    mqttPort = DEFAULT_MQTT_PORT;
    strcpy(mqttTopic, localHost);

    strcpy(accessPointPassword, DEFAULT_AP_PASSWORD);
    strcpy(nodeFriendlyName, DEFAULT_NODE_FRIENDLY_NAME);
    heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
    timeZone = DEFAULT_TIMEZONE;

    clockMode = DEFAULT_CLOCK_MODE;

    if (!Save())
    {
        Serial.println("Failed to save config!");
    }
    else
    {
        ESP.restart();
#ifdef __debugSettings
        Serial.println("Settings saved.");
#endif
    }
}

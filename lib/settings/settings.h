#ifndef SETTINGS_H
#define SETTINGS_H

#define __debugSettings

#include <Arduino.h>

#define DEFAULT_ADMIN_PASSWORD "admin"
#define DEFAULT_NODE_FRIENDLY_NAME "Ksipna"
#define DEFAULT_AP_PASSWORD "esp12345678"

#define DEFAULT_TIMEZONE 13
#define DEFAULT_HEARTBEAT_INTERVAL 300 //  seconds

#define DEFAULT_DURATION 60

#ifdef __debugSettings
#define DEFAULT_MQTT_SERVER "192.168.123.42"
#else
#define DEFAULT_MQTT_SERVER "test.mosquitto.org"
#endif

#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_TOPIC "vNode"
#define DEFAULT_CLOCK_MODE 0

class settings
{
public:
    //  Saved values
    char wifiSSID[22];
    char wifiPassword[32];

    char adminPassword[32];

    char accessPointPassword[32];

    char nodeFriendlyName[32];
    u_int heartbeatInterval;

    signed char timeZone;

    char mqttServer[64];
    int mqttPort;
    char mqttTopic[32];

    int clockMode;

    //  Calculated values
    char localHost[32];

    //------------------------------------------------------------

    settings();

    String GetDeviceMAC();
    bool Load();
    bool Save();
    void LoadDefaults();
};

#endif
#include "main.h"
#include "version.h"
#include "common.h"
#include "mqtt.h"
#include "leds.h"
#include "connection.h"
#include "daisy595.h"

settings appSettings;
daisy595 disp;

unsigned long oldMillis = 0;

void setup()
{
    Serial.begin(DEBUG_SPEED);

    String FirmwareVersionString = String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);

    Serial.printf("\r\n\n\nBooting ESP node %u...\r\n", ESP.getChipId());
    Serial.println("Hardware ID:      " + HARDWARE_ID);
    Serial.println("Hardware version: " + HARDWARE_VERSION);
    Serial.println("Software ID:      " + FIRMWARE_ID);
    Serial.println("Software version: " + FirmwareVersionString);
    Serial.println();

    setupCommon();

    if (!appSettings.Load())
        appSettings.LoadDefaults();

    setupLEDs();
    setupMQTT();
    disp.DoBootAnimation();

    //  Finished setup()
    Serial.println("Setup finished successfully.");
}

void loop()
{
    loopConnection();
}
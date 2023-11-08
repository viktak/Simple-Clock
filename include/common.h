#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <os_type.h>

// #define __debugSettings
#define SerialMon Serial

static const int32_t DEBUG_SPEED = 115200;
static const String HARDWARE_ID = "ESP-12E-12F BB";
static const String HARDWARE_VERSION = "2.0";
static const String FIRMWARE_ID = "Ksipna";

extern char *GetFullDateTime(const char *formattingString, size_t size);
extern String DateTimeToString(const time_t time);
extern String TimeIntervalToString(const time_t time);

extern void setupCommon();

#endif
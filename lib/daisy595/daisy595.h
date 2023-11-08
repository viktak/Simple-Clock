#ifndef DAISY595_H
#define DAISY595_H

#include <Arduino.h>

#define DAISY_SER 14
#define DAISY_SRCLK 12
#define DAISY_RCLK 13

// #define FILLER

#ifdef FILLER

#define COMMON_CATHODE

#define MAX_DIGITS 6

#define TEN_HOUR_DIGIT 3
#define HOUR_DIGIT 2
#define TEN_MINUTE_DIGIT 1
#define MINUTE_DIGIT 0

#else

#define COMMON_ANODE
#define MAX_DIGITS 4

#define TEN_HOUR_DIGIT 3
#define HOUR_DIGIT 2
#define TEN_MINUTE_DIGIT 1
#define MINUTE_DIGIT 0

#endif

#define SEGMENTS_PER_DIGIT 8

class daisy595
{
private:
    uint8_t digit[MAX_DIGITS];

#ifdef COMMON_CATHODE
    uint8_t numLookup[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};
    uint8_t wifiLookup[4] = {0b00001000, 0b01000000, 0b00000001, 0b01000000};
#endif

#ifdef COMMON_ANODE
    uint8_t numLookup[10] = {0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000};
    uint8_t wifiLookup[4] = {0b11110111, 0b10111111, 0b11111110, 0b10111111};
#endif

    void clockShiftRegister();
    void clockStorageRegister();

public:
    daisy595();
    void loop();
    void RefreshDisplay();
    void ClearDisplay();
    void SetDigit(uint8_t dig, uint8_t val);
    void SetDigitsToHoursMinutes(time_t t);
    void SetDigitsToMinutesSeconds(time_t t);
    void DoBootAnimation();
    void DoWifiAnimation(uint8_t value);
};

#endif
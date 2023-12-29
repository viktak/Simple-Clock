#include <TimeLib.h>

#include "daisy595.h"

unsigned long dispMillis = 0;
size_t num = 0;

void daisy595::clockShiftRegister()
{
    digitalWrite(DAISY_SRCLK, HIGH);
    digitalWrite(DAISY_SRCLK, LOW);
}

void daisy595::clockStorageRegister()
{
    digitalWrite(DAISY_RCLK, HIGH);
    digitalWrite(DAISY_RCLK, LOW);
}

void daisy595::RefreshDisplay()
{
    for (size_t d = 0; d < MAX_DIGITS; d++)
    {
        for (size_t s = 0; s < SEGMENTS_PER_DIGIT; s++)
        {
            uint8_t seg = digit[d] & (1 << s);
            digitalWrite(DAISY_SER, seg);
            clockShiftRegister();
        }
    }
    clockStorageRegister();
}

void daisy595::ClearDisplay()
{
    for (size_t i = 0; i < MAX_DIGITS; i++)
    {
#ifdef COMMON_CATHODE
        digit[i] = 0b00000000;
#endif
#ifdef COMMON_ANODE
        digit[i] = 0b11111111;
#endif
    }
    RefreshDisplay();
}

daisy595::daisy595()
{
    pinMode(DAISY_SER, OUTPUT);
    pinMode(DAISY_SRCLK, OUTPUT);
    pinMode(DAISY_RCLK, OUTPUT);

    digitalWrite(DAISY_SER, LOW);
    digitalWrite(DAISY_SRCLK, LOW);
    digitalWrite(DAISY_RCLK, LOW);

    ClearDisplay();
}

void daisy595::loop()
{
    if (millis() - dispMillis > 500)
    {
        for (size_t d = 0; d < MAX_DIGITS; d++)
        {
            SetDigit(d, num);
        }
        RefreshDisplay();
        num++;
        if (num > 9)
            num = 0;

        dispMillis = millis();
    }
}

void daisy595::SetDigit(uint8_t dig, uint8_t val)
{
    digit[dig] = val;
}

void daisy595::SetDigitsToHoursMinutes(time_t t)
{
    uint8_t ho = hour(t);
    if (ho < 10)
        SetDigit(TEN_HOUR_DIGIT, blankLookup[0]);
    else
        SetDigit(TEN_HOUR_DIGIT, numLookup[ho / 10]);

    SetDigit(HOUR_DIGIT, numLookup[ho % 10]);

    uint8_t min = minute(t);
    if (min < 10)
        SetDigit(TEN_MINUTE_DIGIT, numLookup[0]);
    else
        SetDigit(TEN_MINUTE_DIGIT, numLookup[min / 10]);

    SetDigit(MINUTE_DIGIT, numLookup[min % 10]);
}

void daisy595::SetDigitsToMinutesSeconds(time_t t)
{
    uint8_t min = minute(t);
    if (min < 10)
        SetDigit(3, blankLookup[0]);
    else
        SetDigit(3, numLookup[min / 10]);

    SetDigit(2, numLookup[min % 10]);

    uint8_t sec = second(t);
    if (sec < 10)
        SetDigit(1, numLookup[0]);
    else
        SetDigit(1, numLookup[sec / 10]);

    SetDigit(0, numLookup[sec % 10]);
}

void daisy595::DoBootAnimation()
{
    for (size_t j = 0; j < sizeof(numLookup) / sizeof(uint8_t); j++)
    {
        for (size_t i = 0; i < 4; i++)
        {
            SetDigit(i, numLookup[j]);
        }
        RefreshDisplay();
        delay(250);
    }
    ClearDisplay();
}

void daisy595::DoWifiAnimation(uint8_t value)
{
    for (size_t d = 0; d < 4; d++)
    {
        SetDigit(d, wifiLookup[value % (sizeof(wifiLookup) / sizeof(uint8_t))]);
    }
    RefreshDisplay();
}
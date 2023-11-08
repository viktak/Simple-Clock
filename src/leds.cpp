#include <Esp.h>

#include "leds.h"

void connectionLED_TOGGLE()
{
    if (digitalRead(CONNECTION_STATUS_LED_GPIO) == LOW)
        digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
    else
        digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
}

void connectionLED_ON()
{
    digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
}

void connectionLED_OFF()
{
    digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
}

void setupLEDs()
{
    //  Connection LED
    pinMode(CONNECTION_STATUS_LED_GPIO, OUTPUT);
    digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
}

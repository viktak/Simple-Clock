#ifndef LEDS_H
#define LEDS_H

#define CONNECTION_STATUS_LED_GPIO 0

extern void connectionLED_TOGGLE();
extern void connectionLED_ON();
extern void connectionLED_OFF();

extern void setupLEDs();

#endif
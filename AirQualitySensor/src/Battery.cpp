#include "Battery.h"
#include <Arduino.h>

#define PIN 35 // vbat is on pin35
#define MAX_BATTERY 4.2 // 4.2V => 100%
#define MIN_BATTERY 3.3 // 3.0V => 0%. We can have reboot after 3 successive measures below 3.3V

Battery::Battery()
{

}

void Battery::init()
{
    pinMode(PIN, INPUT);
}

void Battery::measure()
{
    int raw = analogRead(PIN); // 0 => 0.1V, 4095 => 3.2 V
    /* theoritical
    float pinVoltage = 0.1 + (((float)raw * 3.2) / 4095.0);
    voltage = pinVoltage * 2.0; // there is a 100K --- 100K resistor bridge
    */
    /* experimental
        real measure    real load   theoritical voltage   raw
        4.03 V          83%         3.76 V                2278
        3.85 V          53%         3.56 V                2150
        3.73 V          34%         3.50 V                2111
        3.56 V          18%         3.3 V                 1983
    */
    voltage = 0.0016*(float)raw + 0.3356; // from linear regression

    load = 100.0 * (voltage - MIN_BATTERY)/(MAX_BATTERY - MIN_BATTERY);
    if (load < 0) load = 0;
    if (load  > 100) load = 100;
}


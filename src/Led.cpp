#include "Led.h"
#include <Arduino.h>

void Led::init()
{
    pinMode(pin, OUTPUT); // налаштовуємо пін на ВИХІД (led)
}

void Led::handler(bool light_on)
{
    state = light_on;
    digitalWrite(pin, state ? HIGH : LOW);
}

void Led::blink(u32_t halfPeriodMs)
{
    // Меандр 50/50: перемикаємо стан кожні halfPeriodMs без delay()
    if (millis() - lastToggle >= halfPeriodMs)
    {
        lastToggle = millis();
        handler(!state);
    }
}

#pragma once

#include <Arduino.h>

class Led
{
private:
    u8_t pin;
    bool state = false;
    u32_t lastToggle = 0;

public:
    Led(u8_t pin) : pin(pin) {}
    void init();
    void handler(bool light_on);    // просто увімкнути / вимкнути
    void blink(u32_t halfPeriodMs); // неблокуюче блимання, викликати в кожному loop()
};

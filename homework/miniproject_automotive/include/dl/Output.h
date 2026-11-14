#pragma once

#include <Arduino.h>

// DL (Driver Layer): цифровий вихід — LED або реле
class Output
{
public:
    void init(u8_t outputPin)
    {
        pin = outputPin;
        pinMode(pin, OUTPUT);
        set(false);
    }

    void set(bool on)
    {
        state = on;
        digitalWrite(pin, on ? HIGH : LOW);
    }

    bool get() const { return state; }

private:
    u8_t pin = 0;
    bool state = false;
};

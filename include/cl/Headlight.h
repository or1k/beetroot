#pragma once

#include "dl/Output.h"

// CL (Control Layer): фари. Стан: OFF / ON.
class Headlight
{
public:
    void init(u8_t pin) { led.init(pin); }

    void set(bool on)
    {
        led.set(on);
        Serial.printf("[CL] Headlight: %s\n", on ? "ON" : "OFF");
    }

private:
    Output led;
};

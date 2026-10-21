#pragma once

#include <Arduino.h>

// Стан світлодіода
enum class LedState
{
    Off,
    On
};

class Led
{
private:
    u8_t pin;
    LedState state = LedState::Off;

public:
    Led(u8_t ledPin) { pin = ledPin; }

    void init();
    void set(LedState newState);
    void toggle(); // перемкнути: On -> Off, Off -> On
};

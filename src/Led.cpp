#include "Led.h"
#include <Arduino.h>

void Led::init()
{
    pinMode(pin_, OUTPUT);
    set(LedState::Off);
}

void Led::set(LedState state)
{
    state_ = state;
    digitalWrite(pin_, state_ == LedState::On ? HIGH : LOW);
}

void Led::toggle()
{
    set(state_ == LedState::On ? LedState::Off : LedState::On);
}

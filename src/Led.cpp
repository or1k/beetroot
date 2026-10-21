#include "Led.h"

void Led::init()
{
    pinMode(pin, OUTPUT); // налаштовуємо пін на ВИХІД
    set(LedState::Off);
}

void Led::set(LedState newState)
{
    state = newState;

    if (state == LedState::On)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        digitalWrite(pin, LOW);
    }
}

void Led::toggle()
{
    if (state == LedState::On)
    {
        set(LedState::Off);
    }
    else
    {
        set(LedState::On);
    }
}

#include "dl/Button.h"
#include "config.h"

void Button::init(u8_t buttonPin, ButtonListener *buttonListener)
{
    pin = buttonPin;
    listener = buttonListener;
    pinMode(pin, INPUT_PULLUP); // відпущена = HIGH, натиснута = LOW
    debounceTimer.init(this, EventType::DebounceDone, "debounce");
    attachInterruptArg(digitalPinToInterrupt(pin), isr, this, CHANGE);
}

// Переривання: мінімальне — лише кладемо подію в чергу
void IRAM_ATTR Button::isr(void *arg)
{
    postEventFromISR(static_cast<Button *>(arg), EventType::Edge);
}

void Button::handleEvent(EventType type)
{
    if (type == EventType::Edge)
    {
        // Кожен фронт (у т.ч. брязкіт) перезапускає таймер.
        // Таймер спрацює лише тоді, коли фронтів не буде DEBOUNCE_MS — рівень стабільний.
        debounceTimer.startOnce(DEBOUNCE_MS);
    }
    else if (type == EventType::DebounceDone)
    {
        bool pressedNow = digitalRead(pin) == LOW;

        if (pressedNow && state == State::Released)
        {
            state = State::Pressed;
            listener->onPress();
        }
        else if (!pressedNow && state == State::Pressed)
        {
            state = State::Released;
            listener->onRelease();
        }
    }
}

#include "Button.h"
#include "Config.h"

volatile bool Button::pressedFlag_ = false;

void IRAM_ATTR Button::isr()
{
    // Мінімальний ISR: без Serial, без затримок, без логіки
    pressedFlag_ = true;
}

void Button::init()
{
    // Внутрішня підтяжка: у спокої HIGH, натиснуто (замкнуто на GND) — LOW
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), isr, FALLING);
}

bool Button::poll(uint32_t nowMs)
{
    const bool isPressed = digitalRead(pin_) == LOW;

    // «Озброюємо» кнопку, лише коли вона стабільно відпущена kDebounceMs.
    // Так відсікаються і брязкіт при натисканні, і хибні FALLING при відпусканні.
    if (isPressed)
    {
        releasedSinceMs_ = nowMs;
    }
    else if (!armed_ && nowMs - releasedSinceMs_ >= Config::kDebounceMs)
    {
        armed_ = true;
    }

    if (!pressedFlag_)
    {
        return false;
    }
    pressedFlag_ = false; // bool пишеться атомарно, критична секція не потрібна

    if (armed_ && isPressed)
    {
        armed_ = false;
        return true;
    }
    return false; // брязкіт або імпульс уже відпущеної кнопки
}

#pragma once

#include "events.h"

// DL (Driver Layer): кнопка. Переривання на кожен фронт + таймер антидребезгу.
// Нагору (у CL) віддає лише чисті події "натиснули" / "відпустили".

class ButtonListener
{
public:
    virtual void onPress() = 0;
    virtual void onRelease() = 0;
};

class Button : public EventHandler
{
public:
    void init(u8_t pin, ButtonListener *listener);
    void handleEvent(EventType type) override;

private:
    static void IRAM_ATTR isr(void *arg);

    // Стан: RELEASED / PRESSED — останній стабільний рівень після антидребезгу
    enum class State
    {
        Released,
        Pressed
    };

    u8_t pin = 0;
    ButtonListener *listener = nullptr;
    State state = State::Released;
    Timer debounceTimer;
};

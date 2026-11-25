#pragma once

#include "events.h"
#include "cl/Sma.h"

// CL (Control Layer): педаль газу на потенціометрі. Вимірювання за таймером (20 мс) -> SMA -> 0..100 %.
// Нагору віддає значення лише тоді, коли воно змінилось (щоб не засмічувати лог).

class ThrottleListener
{
public:
    virtual void onThrottle(u8_t percent) = 0;
};

class Throttle : public EventHandler
{
public:
    void init(u8_t adcPin, ThrottleListener *throttleListener)
    {
        pin = adcPin;
        listener = throttleListener;
        timer.init(this, EventType::AdcTick, "pot");
        timer.startPeriodic(ADC_PERIOD_MS);
    }

    void handleEvent(EventType type) override
    {
        if (type != EventType::AdcTick)
        {
            return;
        }
        u16_t filtered = filter.update(analogRead(pin));
        u8_t percent = (u32_t)filtered * 100 / 4095;

        if (percent != lastPercent)
        {
            lastPercent = percent;
            listener->onThrottle(percent);
        }
    }

private:
    u8_t pin = 0;
    ThrottleListener *listener = nullptr;
    u8_t lastPercent = 255; // 255 — щоб перше значення точно передалось
    Sma filter;
    Timer timer;
};

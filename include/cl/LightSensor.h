#pragma once

#include "events.h"
#include "cl/Sma.h"

// CL (Control Layer): датчик освітленості. Вимірювання за таймером (20 мс) -> SMA -> гістерезис.
// Стан: LIGHT / DARK. Нагору віддає лише зміну стану.

class LightListener
{
public:
    virtual void onLightChanged(bool dark) = 0;
};

class LightSensor : public EventHandler
{
public:
    void init(u8_t adcPin, LightListener *lightListener)
    {
        pin = adcPin;
        listener = lightListener;
        timer.init(this, EventType::AdcTick, "ldr");
        timer.startPeriodic(ADC_PERIOD_MS);
    }

    void handleEvent(EventType type) override
    {
        if (type != EventType::AdcTick)
        {
            return;
        }
        u16_t filtered = filter.update(analogRead(pin));

        // Два пороги: між ними стан не змінюється — фари не мерехтять
        if (state == State::Light && filtered < LIGHT_TH_DARK)
        {
            state = State::Dark;
            Serial.printf("[CL] Light sensor: DARK (SMA %u)\n", filtered);
            listener->onLightChanged(true);
        }
        else if (state == State::Dark && filtered > LIGHT_TH_LIGHT)
        {
            state = State::Light;
            Serial.printf("[CL] Light sensor: LIGHT (SMA %u)\n", filtered);
            listener->onLightChanged(false);
        }
    }

private:
    enum class State
    {
        Light,
        Dark
    };

    u8_t pin = 0;
    LightListener *listener = nullptr;
    State state = State::Light;
    Sma filter;
    Timer timer;
};

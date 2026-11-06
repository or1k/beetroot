#pragma once

#include "events.h"
#include "dl/Output.h"
#include "config.h"

// CL (Control Layer): стоп-сигнал. Стан: OFF / ON (на BRAKE_LIGHT_MS, потім сам гасне).
class BrakeLight : public EventHandler
{
public:
    void init(u8_t ledPin)
    {
        led.init(ledPin);
        offTimer.init(this, EventType::BrakeTimeout, "brake");
    }

    void flash()
    {
        led.set(true);
        offTimer.startOnce(BRAKE_LIGHT_MS); // повторний удар просто продовжує час
        Serial.println("[CL] Brake light: ON");
    }

    void handleEvent(EventType type) override
    {
        if (type == EventType::BrakeTimeout)
        {
            led.set(false);
            Serial.println("[CL] Brake light: OFF");
        }
    }

private:
    Output led;
    Timer offTimer;
};

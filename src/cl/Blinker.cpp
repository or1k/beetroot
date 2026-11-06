#include "cl/Blinker.h"
#include "config.h"

void Blinker::init(u8_t leftPin, u8_t rightPin)
{
    left.init(leftPin);
    right.init(rightPin);
    blinkTimer.init(this, EventType::BlinkTick, "blink");
}

void Blinker::setMode(BlinkMode newMode)
{
    mode = newMode;
    blinkTimer.stop();

    const char *names[] = {"OFF", "LEFT", "RIGHT", "HAZARD"};
    Serial.printf("[CL] Blinker: %s\n", names[(int)mode]);

    if (mode == BlinkMode::Off)
    {
        lightOn = false;
    }
    else
    {
        lightOn = true; // одразу засвітити, далі — кожні BLINK_MS
        blinkTimer.startPeriodic(BLINK_MS);
    }
    apply();
}

void Blinker::handleEvent(EventType type)
{
    if (type == EventType::BlinkTick)
    {
        lightOn = !lightOn;
        apply();
    }
}

void Blinker::apply()
{
    bool leftActive = mode == BlinkMode::Left || mode == BlinkMode::Hazard;
    bool rightActive = mode == BlinkMode::Right || mode == BlinkMode::Hazard;
    left.set(lightOn && leftActive);
    right.set(lightOn && rightActive);
}

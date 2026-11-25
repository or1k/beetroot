#pragma once

#include "events.h"
#include "dl/Output.h"

// CL (Control Layer): поворотники та аварійка. Блимання на таймері, 500 мс.

enum class BlinkMode
{
    Off,
    Left,
    Right,
    Hazard
};

class Blinker : public EventHandler
{
public:
    void init(u8_t leftPin, u8_t rightPin);
    void setMode(BlinkMode newMode);
    BlinkMode getMode() const { return mode; }
    void handleEvent(EventType type) override;

private:
    void apply();

    Output left;
    Output right;
    BlinkMode mode = BlinkMode::Off;
    bool lightOn = false; // фаза блимання
    Timer blinkTimer;
};

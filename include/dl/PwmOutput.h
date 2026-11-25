#pragma once

#include <Arduino.h>

// DL (Driver Layer): ШІМ-вихід на LEDC — свій канал і таймер (частота), розрядність 10 біт (0..1023)
class PwmOutput
{
public:
    void init(u8_t pin, u8_t ledcChannel, u32_t freqHz, u8_t bits = 10)
    {
        channel = ledcChannel;
        ledcSetup(channel, freqHz, bits);
        ledcAttachPin(pin, channel);
        set(0);
    }

    void set(u32_t duty)
    {
        ledcWrite(channel, duty);
    }

private:
    u8_t channel = 0;
};

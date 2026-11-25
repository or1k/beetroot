#pragma once

#include "dl/PwmOutput.h"
#include "config.h"

// DL (Driver Layer): серво. ШІМ 50 Гц (період 20 мс), кут задається шириною імпульсу 500..2500 мкс.
class Servo
{
public:
    void init(u8_t pin, u8_t ledcChannel)
    {
        pwm.init(pin, ledcChannel, 50, 14); // 14 біт: 0..16383 на 20 мс
    }

    void setAngle(u8_t angle)
    {
        u32_t pulseUs = SERVO_MIN_US + (u32_t)(SERVO_MAX_US - SERVO_MIN_US) * angle / 180;
        pwm.set(pulseUs * 16383 / 20000);
    }

private:
    PwmOutput pwm;
};

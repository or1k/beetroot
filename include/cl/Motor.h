#pragma once

#include "dl/PwmOutput.h"
#include "config.h"

// CL (Control Layer): двигун. Стан: STOPPED / RUNNING.
// Швидкість 0..100 % -> заповнення ШІМ з мертвою зоною і мінімальним стартовим заповненням (ДЗ 3.3).
class Motor
{
public:
    void init(u8_t pin)
    {
        pwm.init(pin, MOTOR_LEDC_CHANNEL, MOTOR_PWM_FREQ);
    }

    void setSpeed(u8_t percent)
    {
        u32_t duty = 0;
        if (percent >= MOTOR_START_PCT)
        {
            duty = MOTOR_MIN_DUTY + (u32_t)(1023 - MOTOR_MIN_DUTY) * (percent - MOTOR_START_PCT) / (100 - MOTOR_START_PCT);
        }
        pwm.set(duty);

        State newState = duty > 0 ? State::Running : State::Stopped;
        if (newState != state)
        {
            state = newState;
            Serial.printf("[CL] Motor: %s\n", state == State::Running ? "RUNNING" : "STOPPED");
        }
    }

    void stop() { setSpeed(0); }

private:
    enum class State
    {
        Stopped,
        Running
    };

    PwmOutput pwm;
    State state = State::Stopped;
};

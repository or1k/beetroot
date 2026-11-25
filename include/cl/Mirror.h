#pragma once

#include "dl/Servo.h"
#include "config.h"

// CL (Control Layer): бокове дзеркало на серво. Стан: LEFT_LIMIT / MIDDLE / RIGHT_LIMIT.
class Mirror
{
public:
    void init(u8_t servoPin)
    {
        servo.init(servoPin, SERVO_LEDC_CHANNEL);
        center();
    }

    // Один клік енкодера: зсунути дзеркало на MIRROR_STEP градусів у межах
    void step(int8_t direction)
    {
        int newAngle = angle + direction * MIRROR_STEP;
        if (newAngle < MIRROR_MIN_ANGLE)
        {
            newAngle = MIRROR_MIN_ANGLE;
        }
        if (newAngle > MIRROR_MAX_ANGLE)
        {
            newAngle = MIRROR_MAX_ANGLE;
        }
        setAngle(newAngle);
    }

    void center() { setAngle(MIRROR_CENTER); }

private:
    enum class State
    {
        LeftLimit,
        Middle,
        RightLimit
    };

    void setAngle(int newAngle)
    {
        angle = newAngle;
        servo.setAngle(angle);

        state = angle <= MIRROR_MIN_ANGLE   ? State::LeftLimit
                : angle >= MIRROR_MAX_ANGLE ? State::RightLimit
                                            : State::Middle;
        const char *names[] = {"LEFT LIMIT", "MIDDLE", "RIGHT LIMIT"};
        Serial.printf("[CL] Mirror: %d deg (%s)\n", angle, names[(int)state]);
    }

    Servo servo;
    int angle = MIRROR_CENTER;
    State state = State::Middle;
};

#pragma once

#include "cl/ClickDetector.h"
#include "cl/Blinker.h"
#include "cl/Lock.h"
#include "cl/BrakeLight.h"
#include "cl/LightSensor.h"
#include "cl/Throttle.h"
#include "cl/Headlight.h"
#include "cl/Motor.h"
#include "cl/Mirror.h"
#include "dl/Encoder.h"

// BL (Business Layer): логіка автомобіля. Отримує події з CL (кліки, світло, газ) і з енкодера,
// і керує компонентами CL (замок, поворотники, стоп-сигнал, фари, двигун, дзеркало).
//
// Стани:
//   NORMAL — звичайна робота;
//   VALET  — валет-режим (довге натискання замка): замок відкритий, аварійне блокування знято;
//   CRASH  — аварія (довгий удар): двері відкриваються, вмикається аварійка, двигун глушиться,
//            кнопки замка/аварійки/повороту заблоковані. Вийти можна лише через валет-режим.
class Car : public ClickListener, public LightListener, public ThrottleListener, public EncoderListener
{
public:
    struct Parts
    {
        Lock *lock;
        Blinker *blinker;
        BrakeLight *brake;
        Headlight *headlight;
        Motor *motor;
        Mirror *mirror;
    };

    void init(const Parts &carParts);

    void onClick(ButtonId button, Click click) override;
    void onLightChanged(bool dark) override;
    void onThrottle(u8_t percent) override;
    void onRotate(int8_t direction) override;

private:
    enum class State
    {
        Normal,
        Valet,
        Crash
    };

    void setState(State newState);
    void onLockButton(Click click);
    void onHazardButton(Click click);
    void onTurnButton(Click click);
    void onCrashSensor(Click click);

    Parts parts = {};
    State state = State::Normal;
    u8_t throttle = 0; // останнє положення педалі газу — щоб відновити швидкість після аварії
};

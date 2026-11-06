#pragma once

#include "cl/ClickDetector.h"
#include "cl/Blinker.h"
#include "cl/Lock.h"
#include "cl/BrakeLight.h"

// BL (Business Layer): логіка автомобіля. Отримує кліки з CL і керує компонентами CL.
//
// Стани:
//   NORMAL — звичайна робота;
//   VALET  — валет-режим (довге натискання замка): замок відкритий, аварійне блокування знято;
//   CRASH  — аварія (довгий удар): двері відкриті, аварійка ввімкнена, кнопки замка/аварійки/повороту
//            заблоковані. Вийти можна лише через валет-режим.
class Car : public ClickListener
{
public:
    void init(Lock *lock, Blinker *blinker, BrakeLight *brake);
    void onClick(ButtonId button, Click click) override;

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

    Lock *lock = nullptr;
    Blinker *blinker = nullptr;
    BrakeLight *brake = nullptr;
    State state = State::Normal;
};

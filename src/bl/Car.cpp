#include "bl/Car.h"

void Car::init(Lock *carLock, Blinker *carBlinker, BrakeLight *carBrake)
{
    lock = carLock;
    blinker = carBlinker;
    brake = carBrake;
    setState(State::Normal);
}

void Car::setState(State newState)
{
    state = newState;
    const char *names[] = {"NORMAL", "VALET", "CRASH"};
    Serial.printf("[BL] Car state: %s\n", names[(int)state]);
}

void Car::onClick(ButtonId button, Click click)
{
    const char *buttons[] = {"lock", "hazard", "turn", "crash sensor"};
    const char *clicks[] = {"short", "long", "double"};
    Serial.printf("[BL] %s: %s\n", buttons[(int)button], clicks[(int)click]);

    switch (button)
    {
    case ButtonId::Lock:
        onLockButton(click);
        break;
    case ButtonId::Hazard:
        onHazardButton(click);
        break;
    case ButtonId::Turn:
        onTurnButton(click);
        break;
    case ButtonId::Crash:
        onCrashSensor(click);
        break;
    }
}

// 1) Замок: коротке — відкрити/закрити, довге — валет-режим (знімає аварійне блокування)
void Car::onLockButton(Click click)
{
    if (click == Click::Short)
    {
        if (state == State::Crash)
        {
            Serial.println("[BL] Blocked: crash mode, doors stay open (use valet mode)");
            return;
        }
        lock->toggle();
    }
    else if (click == Click::Long)
    {
        if (state == State::Valet)
        {
            setState(State::Normal); // повторне довге натискання — вихід з валет-режиму
            return;
        }
        if (state == State::Crash)
        {
            blinker->setMode(BlinkMode::Off); // аварійне блокування знято
        }
        lock->open();
        setState(State::Valet);
    }
}

// 2) Аварійка: коротке — увімк/вимк
void Car::onHazardButton(Click click)
{
    if (click != Click::Short)
    {
        return;
    }
    if (state == State::Crash)
    {
        Serial.println("[BL] Blocked: hazard stays on in crash mode");
        return;
    }
    blinker->setMode(blinker->getMode() == BlinkMode::Hazard ? BlinkMode::Off : BlinkMode::Hazard);
}

// 3) Поворотник: клік — лівий, подвійний клік — правий (повторний клік — вимкнути)
void Car::onTurnButton(Click click)
{
    if (state == State::Crash || blinker->getMode() == BlinkMode::Hazard)
    {
        Serial.println("[BL] Blocked: hazard is on");
        return;
    }

    if (click == Click::Short)
    {
        blinker->setMode(blinker->getMode() == BlinkMode::Left ? BlinkMode::Off : BlinkMode::Left);
    }
    else if (click == Click::Double)
    {
        blinker->setMode(blinker->getMode() == BlinkMode::Right ? BlinkMode::Off : BlinkMode::Right);
    }
}

// 4) Датчик удару: коротке — пригальмовування (стоп-сигнал), довге — аварія
void Car::onCrashSensor(Click click)
{
    if (click == Click::Short)
    {
        brake->flash();
    }
    else if (click == Click::Long && state != State::Crash)
    {
        lock->open();
        blinker->setMode(BlinkMode::Hazard);
        setState(State::Crash);
    }
}

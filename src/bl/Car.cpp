#include "bl/Car.h"

void Car::init(const Parts &carParts)
{
    parts = carParts;
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
    const char *buttons[] = {"lock", "hazard", "turn", "crash sensor", "mirror button"};
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
    case ButtonId::Mirror:
        if (click == Click::Short)
        {
            parts.mirror->center(); // кнопка енкодера — дзеркало в центр
        }
        break;
    }
}

// =====================================================================
// Модуль 2
// =====================================================================

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
        parts.lock->toggle();
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
            parts.blinker->setMode(BlinkMode::Off); // аварійне блокування знято
            parts.motor->setSpeed(throttle);        // двигун знову слухається педалі
        }
        parts.lock->open();
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
    parts.blinker->setMode(parts.blinker->getMode() == BlinkMode::Hazard ? BlinkMode::Off : BlinkMode::Hazard);
}

// 3) Поворотник: клік — лівий, подвійний клік — правий (повторний клік — вимкнути)
void Car::onTurnButton(Click click)
{
    if (state == State::Crash || parts.blinker->getMode() == BlinkMode::Hazard)
    {
        Serial.println("[BL] Blocked: hazard is on");
        return;
    }

    if (click == Click::Short)
    {
        parts.blinker->setMode(parts.blinker->getMode() == BlinkMode::Left ? BlinkMode::Off : BlinkMode::Left);
    }
    else if (click == Click::Double)
    {
        parts.blinker->setMode(parts.blinker->getMode() == BlinkMode::Right ? BlinkMode::Off : BlinkMode::Right);
    }
}

// 4) Датчик удару: коротке — пригальмовування (стоп-сигнал), довге — аварія
void Car::onCrashSensor(Click click)
{
    if (click == Click::Short)
    {
        parts.brake->flash();
    }
    else if (click == Click::Long && state != State::Crash)
    {
        parts.lock->open();
        parts.blinker->setMode(BlinkMode::Hazard);
        parts.motor->stop(); // при аварії двигун глушиться
        setState(State::Crash);
    }
}

// =====================================================================
// Модуль 3
// =====================================================================

// 1) Автоматичні фари: темно — увімкнути, світло — вимкнути
void Car::onLightChanged(bool dark)
{
    parts.headlight->set(dark);
}

// 3) Педаль газу (потенціометр) — швидкість двигуна. В аварії двигун заглушений.
void Car::onThrottle(u8_t percent)
{
    throttle = percent;
    if (state == State::Crash)
    {
        return;
    }
    parts.motor->setSpeed(percent);
}

// 2) Енкодер — бокове дзеркало
void Car::onRotate(int8_t direction)
{
    parts.mirror->step(direction);
}

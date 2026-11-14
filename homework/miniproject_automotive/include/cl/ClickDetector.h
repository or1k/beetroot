#pragma once

#include "dl/Button.h"

// CL (Control Layer): перетворює "натиснули/відпустили" на короткий, довгий або подвійний клік

enum class Click
{
    Short,
    Long,
    Double
};

enum class ButtonId
{
    Lock,
    Hazard,
    Turn,
    Crash
};

class ClickListener
{
public:
    virtual void onClick(ButtonId button, Click click) = 0;
};

class ClickDetector : public ButtonListener, public EventHandler
{
public:
    // detectDouble = false: короткий клік віддається одразу після відпускання, без очікування
    void init(ButtonId id, bool detectDouble, ClickListener *listener);

    void onPress() override;
    void onRelease() override;
    void handleEvent(EventType type) override;

private:
    enum class State
    {
        Idle,          // кнопка відпущена
        Pressed,       // натиснута, йде таймер довгого натискання
        LongHeld,      // довге натискання вже віддали, чекаємо відпускання
        WaitSecond,    // відпустили після короткого — чекаємо другий клік
        SecondPressed  // натиснули вдруге
    };

    void emit(Click click);

    ButtonId id = ButtonId::Lock;
    bool detectDouble = false;
    ClickListener *listener = nullptr;
    State state = State::Idle;
    Timer longTimer;
    Timer doubleTimer;
};

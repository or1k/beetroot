#include "cl/ClickDetector.h"
#include "config.h"

void ClickDetector::init(ButtonId buttonId, bool doubleClicks, ClickListener *clickListener)
{
    id = buttonId;
    detectDouble = doubleClicks;
    listener = clickListener;
    longTimer.init(this, EventType::LongTimeout, "long");
    doubleTimer.init(this, EventType::DoubleTimeout, "double");
}

void ClickDetector::emit(Click click)
{
    listener->onClick(id, click);
}

void ClickDetector::onPress()
{
    if (state == State::Idle)
    {
        state = State::Pressed;
        longTimer.startOnce(LONG_PRESS_MS);
    }
    else if (state == State::WaitSecond)
    {
        doubleTimer.stop();
        state = State::SecondPressed;
    }
}

void ClickDetector::onRelease()
{
    if (state == State::Pressed)
    {
        longTimer.stop();
        if (detectDouble)
        {
            state = State::WaitSecond; // може, буде другий клік
            doubleTimer.startOnce(DOUBLE_CLICK_MS);
        }
        else
        {
            state = State::Idle;
            emit(Click::Short);
        }
    }
    else if (state == State::SecondPressed)
    {
        state = State::Idle;
        emit(Click::Double);
    }
    else if (state == State::LongHeld)
    {
        state = State::Idle; // довге вже віддали при натисканні
    }
}

void ClickDetector::handleEvent(EventType type)
{
    if (type == EventType::LongTimeout && state == State::Pressed)
    {
        state = State::LongHeld;
        emit(Click::Long); // довге спрацьовує одразу через 2000 мс, не чекаючи відпускання
    }
    else if (type == EventType::DoubleTimeout && state == State::WaitSecond)
    {
        state = State::Idle;
        emit(Click::Short); // другого кліку не було — це був звичайний клік
    }
}

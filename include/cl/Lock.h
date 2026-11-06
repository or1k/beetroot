#pragma once

#include "dl/Output.h"

// CL (Control Layer): замок дверей на реле. Стан: LOCKED / UNLOCKED.
class Lock
{
public:
    void init(u8_t relayPin)
    {
        relay.init(relayPin);
        set(State::Locked);
    }

    void open() { set(State::Unlocked); }
    void close() { set(State::Locked); }
    void toggle() { set(state == State::Locked ? State::Unlocked : State::Locked); }

private:
    enum class State
    {
        Locked,
        Unlocked
    };

    void set(State newState)
    {
        state = newState;
        relay.set(state == State::Unlocked); // реле увімкнене — замок відкритий
        Serial.printf("[CL] Lock: %s\n", state == State::Locked ? "LOCKED" : "UNLOCKED");
    }

    Output relay;
    State state = State::Locked;
};

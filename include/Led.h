#pragma once

#include <cstdint>

// Стан світлодіода: типобезпечно, не змішується з int чи bool
enum class LedState : uint8_t
{
    Off,
    On
};

class Led
{
public:
    explicit constexpr Led(uint8_t pin) : pin_(pin) {}

    void init();
    void set(LedState state);
    void toggle();
    LedState state() const { return state_; }

private:
    const uint8_t pin_;
    LedState state_ = LedState::Off;
};

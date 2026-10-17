#pragma once

#include <Arduino.h>
#include "config.h"

class Ldr
{
private:
    u16_t readings[N];
    u8_t  idx = 0;
    u32_t sum = 0;
    u16_t filtered = 0;
    bool ldrState = false; // true — темно
    u32_t lastTimeChangeLdr = 0;

    u16_t movingAvg(u16_t rawData);
    void filterInit(u16_t initData);

public:
    void init();
    void poll();
    bool get_state();   // true — темно (тунель), треба вмикати світло
    u16_t get_value();  // відфільтроване значення АЦП
};

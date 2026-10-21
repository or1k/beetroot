#pragma once
#include <Arduino.h>

class Button
{
private:
    u32_t lastTimeChangeBtn = 0;
    u32_t btnTimekeeper = 0;  // момент натискання (0 — кнопка відпущена)
    bool longPressFired = false; // подія вже спрацювала для цього затискання
    bool longPressEvent = false; // непрочитана подія довгого натискання

public:
    void init();
    void poll();
    bool get_long_press(); // true один раз на кожне довге затискання
};

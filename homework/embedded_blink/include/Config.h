#pragma once

#include <Arduino.h>

// Усі налаштування в одному місці — жодних "магічних чисел" у коді.
// static constexpr — константа, відома ще під час компіляції.
struct Config
{
    static constexpr u8_t LED_PIN = 18;    // LED через резистор 220 Ом на GND
    static constexpr u8_t BUTTON_PIN = 17; // кнопка між GPIO17 і GND

    static constexpr u32_t BLINK_INTERVAL_MS = 500; // LED горить 500 мс і не горить 500 мс
    static constexpr u32_t DEBOUNCE_MS = 50;        // захист від брязкоту кнопки

    static constexpr u32_t SERIAL_SPEED = 115200;
    static constexpr u32_t LOOPS_TO_MEASURE = 1000; // раз на скільки ітерацій друкувати час loop()
};

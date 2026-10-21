#pragma once

#include <cstdint>

// Усі налаштування проєкту в одному місці — жодних «магічних чисел» у коді.
// static constexpr: значення відомі на етапі компіляції, не займають RAM
// і не можуть бути випадково змінені під час роботи.
struct Config
{
    // ---------- Піни ----------
    static constexpr uint8_t kLedPin = 18;    // LED через резистор 220 Ом на GND
    static constexpr uint8_t kButtonPin = 17; // кнопка між GPIO17 і GND, внутрішня підтяжка

    // ---------- Таймінги ----------
    static constexpr uint32_t kBlinkIntervalMs = 500; // півперіод блимання (1 Гц)
    static constexpr uint32_t kDebounceMs = 50;       // антидребезг кнопки

    // ---------- Serial ----------
    static constexpr uint32_t kSerialBaud = 115200;
    static constexpr uint32_t kProfileEveryIterations = 1000; // звіт про час loop()
};

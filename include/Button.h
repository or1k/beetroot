#pragma once

#include <cstdint>
#include <Arduino.h>

// Кнопка з обробкою через переривання.
// ISR лише ставить прапорець; антидребезг і вся логіка — у poll(), тобто в loop().
// Прапорець static, бо ISR — це звичайна функція без this. Тому клас розрахований на одну кнопку.
class Button
{
public:
    explicit constexpr Button(uint8_t pin) : pin_(pin) {}

    void init();

    // Викликати в кожній ітерації loop(). Повертає true один раз на кожне натискання.
    bool poll(uint32_t nowMs);

private:
    static void IRAM_ATTR isr();

    // volatile: змінюється в ISR, тому компілятор не має кешувати її в регістрі
    static volatile bool pressedFlag_;

    const uint8_t pin_;
    bool armed_ = true;          // кнопку відпущено і стабільно — можна приймати нове натискання
    uint32_t releasedSinceMs_ = 0;
};

#pragma once

#include "events.h"

// DL (Driver Layer): енкодер KY-040 (виходи CLK і DT).
// Переривання на кожну зміну CLK/DT декодує квадратурний сигнал за таблицею.
// Брязкіт дає неправильні або "туди-назад" переходи, які взаємно знищуються, —
// тому таблиця сама працює як антидребезг. Нагору віддає лише цілі кліки: +1 / −1.

class EncoderListener
{
public:
    virtual void onRotate(int8_t direction) = 0; // +1 — за годинниковою, −1 — проти
};

class Encoder : public EventHandler
{
public:
    void init(u8_t clkPin, u8_t dtPin, EncoderListener *listener);
    void handleEvent(EventType type) override;

private:
    static void IRAM_ATTR isr(void *arg);
    u8_t IRAM_ATTR readPins();

    u8_t clk = 0;
    u8_t dt = 0;
    EncoderListener *listener = nullptr;
    volatile u8_t lastState = 0; // попередній стан двох пінів (2 біти)
    volatile int8_t steps = 0;   // накопичені переходи всередині одного кліку
};

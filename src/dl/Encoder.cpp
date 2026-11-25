#include "dl/Encoder.h"
#include "config.h"

// Таблиця переходів: індекс = (попередній стан << 2) | новий стан.
// +1 / −1 — правильний крок у той чи інший бік, 0 — неможливий перехід (брязкіт), ігноруємо.
static const int8_t TRANSITIONS[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void Encoder::init(u8_t clkPin, u8_t dtPin, EncoderListener *encoderListener)
{
    clk = clkPin;
    dt = dtPin;
    listener = encoderListener;
    pinMode(clk, INPUT_PULLUP);
    pinMode(dt, INPUT_PULLUP);
    lastState = readPins();
    attachInterruptArg(digitalPinToInterrupt(clk), isr, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(dt), isr, this, CHANGE);
}

// Читаємо обидва піни одразу з регістра входів (піни < 32)
u8_t IRAM_ATTR Encoder::readPins()
{
    u32_t in = GPIO.in;
    return (((in >> clk) & 1) << 1) | ((in >> dt) & 1);
}

void IRAM_ATTR Encoder::isr(void *arg)
{
    Encoder *e = static_cast<Encoder *>(arg);
    u8_t state = e->readPins();
    e->steps += TRANSITIONS[(e->lastState << 2) | state];
    e->lastState = state;

    // Повний клік — кладемо подію в чергу
    if (e->steps >= ENCODER_STEPS_PER_CLICK)
    {
        e->steps = 0;
        postEventFromISR(e, EventType::EncoderCW);
    }
    else if (e->steps <= -ENCODER_STEPS_PER_CLICK)
    {
        e->steps = 0;
        postEventFromISR(e, EventType::EncoderCCW);
    }
}

void Encoder::handleEvent(EventType type)
{
    if (type == EventType::EncoderCW)
    {
        listener->onRotate(+1);
    }
    else if (type == EventType::EncoderCCW)
    {
        listener->onRotate(-1);
    }
}

#pragma once

#include <Arduino.h>
#include "config.h"

// CL (Control Layer): фільтр аналогового сигналу — проста ковзна середня (SMA, ДЗ 3.1)
class Sma
{
public:
    u16_t update(u16_t value)
    {
        if (!ready)
        {
            // Перше вимірювання — заповнюємо весь буфер, щоб середнє одразу було правильним
            for (int i = 0; i < SMA_WINDOW; i++)
            {
                samples[i] = value;
            }
            sum = (u32_t)value * SMA_WINDOW;
            ready = true;
        }
        sum -= samples[index]; // прибрати найстаріше
        sum += value;          // додати нове
        samples[index] = value;
        index = (index + 1) % SMA_WINDOW;
        return sum / SMA_WINDOW;
    }

private:
    u16_t samples[SMA_WINDOW] = {};
    u8_t index = 0;
    u32_t sum = 0;
    bool ready = false;
};

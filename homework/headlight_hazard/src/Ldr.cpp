#include "Ldr.h"
#include "config.h"
#include <Arduino.h>

bool Ldr::get_state()
{
    return ldrState;
}

u16_t Ldr::get_value()
{
    return filtered;
}

void Ldr::init()
{
    pinMode(LDR, INPUT);         // налаштовуємо пін на ВХІД (ldr)
#if LDR_INVERTED
    filterInit(4095 - analogRead(LDR)); // filter init
#else
    filterInit(analogRead(LDR)); // filter init
#endif
}

void Ldr::poll()
{
    if (millis() - lastTimeChangeLdr > LDR_PERIOD_MS)
    {
        lastTimeChangeLdr = millis();
        u16_t adc_value = analogRead(LDR);
#if LDR_INVERTED
        adc_value = 4095 - adc_value; // приводимо до «більше світла — більше значення»
#endif
        filtered = movingAvg(adc_value);

        // Гістерезис: стан змінюється лише після перетину «свого» порогу
        if (filtered < LDR_DARK_THRESHOLD)
        {
            ldrState = true;
        }
        else if (filtered > LDR_LIGHT_THRESHOLD)
        {
            ldrState = false;
        }
    }
}

void Ldr::filterInit(u16_t initData)
{
    for (u8_t i = 0; i < N; i++)
    {
        readings[i] = initData;
        sum += initData;
    }
    filtered = initData;
}

u16_t Ldr::movingAvg(u16_t rawData)
{
    sum -= readings[idx];
    sum += rawData;
    readings[idx] = rawData;
    idx++;

    if (idx >= N)
    {
        idx = 0;
    }

    return sum / N;
}

// ДЗ 3.2: Калібрування даних з ADC — ESP-IDF.
// Потенціометр ~10 кОм як подільник напруги на ADC1. Кожні 100 мс:
//   RAW -> напруга за формулою (U_manual),
//   RAW -> напруга через калібрований АЦП (U_cali, драйвер adc_cali),
//   похибка між ними у відсотках.
//
// Схема: крайні ніжки потенціометра — на 3V3 і GND, середня (повзунок) — на GPIO2 (ADC1_CH1).

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define POT_CHANNEL ADC_CHANNEL_1 // GPIO2 на ESP32-S3
#define ADC_ATTEN ADC_ATTEN_DB_12 // атенюація 12 дБ: вхідний діапазон ~0..3.1 В
#define ADC_BITS ADC_BITWIDTH_12  // розрядність 12 біт: RAW 0..4095

#define ADC_MAX 4095   // максимальне значення RAW при 12 бітах
#define V_REF_MV 3100  // опорна напруга для формули: повна шкала при атенюації 12 дБ, мВ

#define SAMPLE_PERIOD_MS 100 // вимірювання кожні 100 мс
#define HEADER_EVERY 20      // повторювати заголовок таблиці кожні 20 рядків

static void printHeader(void)
{
    printf("\nRAW   U_manual(mV)  U_cali(mV)  Error(%%)\n");
    printf("------------------------------------------\n");
}

void app_main(void)
{
    // ---------- ADC1 в режимі oneshot ----------
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unitConfig = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&unitConfig, &adc);

    adc_oneshot_chan_cfg_t channelConfig = {
        .bitwidth = ADC_BITS,
        .atten = ADC_ATTEN,
    };
    adc_oneshot_config_channel(adc, POT_CHANNEL, &channelConfig);

    // ---------- Калібрування (заводські дані з eFuse, схема curve fitting) ----------
    adc_cali_handle_t cali = NULL;
    adc_cali_curve_fitting_config_t caliConfig = {
        .unit_id = ADC_UNIT_1,
        .chan = POT_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITS,
    };
    if (adc_cali_create_scheme_curve_fitting(&caliConfig, &cali) != ESP_OK)
    {
        printf("ERROR: ADC calibration is not available on this chip\n");
        return;
    }

    printf("ADC calibration test\n");
    printf("Vref (formula): %d mV | attenuation: 12 dB | resolution: 12 bit (0..%d) | period: %d ms\n",
           V_REF_MV, ADC_MAX, SAMPLE_PERIOD_MS);
    printf("Formula: U_manual = RAW * %d / %d\n", V_REF_MV, ADC_MAX);

    int row = 0;
    TickType_t lastWake = xTaskGetTickCount();

    while (1)
    {
        if (row % HEADER_EVERY == 0)
        {
            printHeader();
        }
        row++;

        int raw = 0;
        adc_oneshot_read(adc, POT_CHANNEL, &raw);

        // 1) Напруга за формулою — лінійна, без урахування особливостей конкретного чипа
        float uManual = (float)raw * V_REF_MV / ADC_MAX;

        // 2) Напруга через калібрований АЦП — враховує нелінійність і заводський розкид
        int uCali = 0;
        adc_cali_raw_to_voltage(cali, raw, &uCali);

        // 3) Похибка формули відносно каліброваного значення
        float error = 0;
        if (uCali > 0)
        {
            error = (uManual - uCali) * 100.0f / uCali;
        }

        printf("%4d  %10.1f  %10d  %8.2f\n", raw, uManual, uCali, error);

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

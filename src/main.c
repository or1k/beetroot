// ДЗ 3.1: Проста ковзна середня (SMA) — ESP-IDF.
// LDR у подільнику напруги на ADC1, зчитування в режимі oneshot кожні 20 мс,
// фільтр SMA, світлодіод вмикається в темряві з гістерезисом (два пороги).
//
// Схема: 3V3 -> LDR -> GPIO2 (ADC1_CH1) -> 10 кОм -> GND.
//   Темно -> опір LDR великий -> напруга на GPIO2 мала -> RAW малий.
// LED: GPIO4 -> 220 Ом -> LED -> GND.

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define LDR_CHANNEL ADC_CHANNEL_1 // GPIO2 на ESP32-S3
#define LED_PIN GPIO_NUM_4

#define SAMPLE_PERIOD_MS 20 // 50 вимірювань на секунду
#define SMA_WINDOW 16       // скільки останніх вимірювань усереднюємо
#define LOG_EVERY 10        // друкувати лог кожне 10-те вимірювання (раз на 200 мс)

// Гістерезис: LED вмикається нижче TH_DARK, а вимикається лише вище TH_LIGHT.
// Між порогами стан не змінюється — тому LED не мерехтить на межі освітленості.
// Пороги залежать від кімнати: подивись значення SMA в лозі і підбери.
#define TH_DARK 1500
#define TH_LIGHT 1900

// ---------- SMA: кільцевий буфер + біжуча сума ----------
static uint16_t samples[SMA_WINDOW];
static uint8_t sampleIndex = 0;
static uint32_t sampleSum = 0;

static void smaInit(uint16_t firstValue)
{
    for (int i = 0; i < SMA_WINDOW; i++)
    {
        samples[i] = firstValue;
    }
    sampleSum = (uint32_t)firstValue * SMA_WINDOW;
}

// Додати нове значення і повернути середнє за останні SMA_WINDOW вимірювань
static uint16_t smaUpdate(uint16_t newValue)
{
    sampleSum -= samples[sampleIndex]; // прибрати найстаріше
    sampleSum += newValue;             // додати нове
    samples[sampleIndex] = newValue;
    sampleIndex = (sampleIndex + 1) % SMA_WINDOW;
    return sampleSum / SMA_WINDOW;
}

void app_main(void)
{
    // ---------- LED ----------
    gpio_config_t led = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led);
    gpio_set_level(LED_PIN, 0);

    // ---------- ADC1 в режимі oneshot ----------
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unitConfig = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&unitConfig, &adc);

    adc_oneshot_chan_cfg_t channelConfig = {
        .bitwidth = ADC_BITWIDTH_12, // 0..4095
        .atten = ADC_ATTEN_DB_12,    // діапазон ~0..3.1 В
    };
    adc_oneshot_config_channel(adc, LDR_CHANNEL, &channelConfig);

    // Калібрування — лише щоб показати в лозі напругу в мілівольтах
    adc_cali_handle_t cali = NULL;
    adc_cali_curve_fitting_config_t caliConfig = {
        .unit_id = ADC_UNIT_1,
        .chan = LDR_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    bool caliOk = adc_cali_create_scheme_curve_fitting(&caliConfig, &cali) == ESP_OK;

    // ---------- Старт ----------
    int raw = 0;
    adc_oneshot_read(adc, LDR_CHANNEL, &raw);
    smaInit(raw); // заповнюємо буфер першим значенням, щоб середнє одразу було правильним

    bool ledOn = false;
    int counter = 0;
    TickType_t lastWake = xTaskGetTickCount();

    printf("SMA light sensor: window %d, period %d ms, TH_DARK %d, TH_LIGHT %d\n",
           SMA_WINDOW, SAMPLE_PERIOD_MS, TH_DARK, TH_LIGHT);

    while (1)
    {
        // Одне вимірювання (oneshot): ADC запускається, міряє і повертає результат
        adc_oneshot_read(adc, LDR_CHANNEL, &raw);
        uint16_t sma = smaUpdate(raw);

        // Гістерезис: два пороги
        if (!ledOn && sma < TH_DARK)
        {
            ledOn = true;
            gpio_set_level(LED_PIN, 1);
            printf(">>> DARK  (SMA %u < %d): LED ON\n", sma, TH_DARK);
        }
        else if (ledOn && sma > TH_LIGHT)
        {
            ledOn = false;
            gpio_set_level(LED_PIN, 0);
            printf(">>> LIGHT (SMA %u > %d): LED OFF\n", sma, TH_LIGHT);
        }

        if (++counter >= LOG_EVERY)
        {
            counter = 0;
            int mv = 0;
            if (caliOk)
            {
                adc_cali_raw_to_voltage(cali, sma, &mv);
            }
            printf("raw: %4d | SMA: %4u | %4d mV | LED: %s\n", raw, sma, mv, ledOn ? "ON" : "OFF");
        }

        // Рівний період 20 мс: задача спить до наступного моменту вимірювання
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

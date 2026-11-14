// ДЗ 3.3: Широтно-імпульсна модуляція (PWM) — ESP-IDF.
// Потенціометр керує яскравістю світлодіода і швидкістю двигуна.
//
// Щоб двигун і світлодіод не впливали одне на одного:
//   - у кожного свій таймер LEDC і свій канал (різні частоти ШІМ);
//   - двигун живиться від окремої плати живлення через транзистор, з діодом і конденсатором;
//   - значення потенціометра згладжується SMA, щоб завади від двигуна не "смикали" яскравість.
//
// Схема:
//   Потенціометр: 3V3 — повзунок на GPIO2 (ADC1_CH1) — GND
//   LED:  GPIO4 -> 220 Ом -> LED -> GND
//   Двигун: GPIO17 -> 220 Ом -> база 2N2222; емітер -> GND; колектор -> (−) двигуна;
//           (+) двигуна -> +5 В плати живлення; діод паралельно двигуну смужкою до +; 100 мкФ між + і GND

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#define POT_CHANNEL ADC_CHANNEL_1 // GPIO2
#define LED_PIN 4
#define MOTOR_PIN 17

// ---------- ШІМ ----------
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // заповнення 0..1023
#define PWM_MAX_DUTY 1023
#define LED_PWM_FREQ 5000    // 5 кГц — оку не видно мерехтіння
#define MOTOR_PWM_FREQ 20000 // 20 кГц — двигун не "пищить" (вище за чутний діапазон)

// ---------- Двигун ----------
// Нижче MOTOR_START_PCT потенціометра двигун вимкнений (мертва зона, як у прикладі з уроку).
// Вище — заповнення починається з MOTOR_MIN_DUTY: при меншому двигун гуде, але не крутиться.
#define MOTOR_START_PCT 5
#define MOTOR_MIN_DUTY 400

// ---------- ADC ----------
#define ADC_MAX 4095
#define SAMPLE_PERIOD_MS 20 // 50 разів на секунду
#define SMA_WINDOW 8
#define LOG_EVERY 10 // лог раз на 200 мс

static uint16_t samples[SMA_WINDOW];
static uint8_t sampleIndex = 0;
static uint32_t sampleSum = 0;

static void smaInit(uint16_t value)
{
    for (int i = 0; i < SMA_WINDOW; i++)
    {
        samples[i] = value;
    }
    sampleSum = (uint32_t)value * SMA_WINDOW;
}

static uint16_t smaUpdate(uint16_t value)
{
    sampleSum -= samples[sampleIndex];
    sampleSum += value;
    samples[sampleIndex] = value;
    sampleIndex = (sampleIndex + 1) % SMA_WINDOW;
    return sampleSum / SMA_WINDOW;
}

// Налаштувати окремий таймер і канал LEDC для одного виходу
static void pwmInit(ledc_timer_t timer, ledc_channel_t channel, int pin, uint32_t freq)
{
    ledc_timer_config_t timerConfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = timer,
        .freq_hz = freq,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timerConfig);

    ledc_channel_config_t channelConfig = {
        .gpio_num = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = channel,
        .timer_sel = timer,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&channelConfig);
}

static void pwmSet(ledc_channel_t channel, uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

// Потенціометр (0..4095) -> заповнення двигуна з мертвою зоною і мінімальним стартовим заповненням
static uint32_t motorDutyFromPot(uint16_t pot)
{
    uint32_t percent = (uint32_t)pot * 100 / ADC_MAX;
    if (percent < MOTOR_START_PCT)
    {
        return 0;
    }
    return MOTOR_MIN_DUTY + (uint32_t)(PWM_MAX_DUTY - MOTOR_MIN_DUTY) * (percent - MOTOR_START_PCT) / (100 - MOTOR_START_PCT);
}

void app_main(void)
{
    // Два незалежні ШІМ: свій таймер і канал для LED і для двигуна
    pwmInit(LEDC_TIMER_0, LEDC_CHANNEL_0, LED_PIN, LED_PWM_FREQ);
    pwmInit(LEDC_TIMER_1, LEDC_CHANNEL_1, MOTOR_PIN, MOTOR_PWM_FREQ);

    // ADC1 oneshot для потенціометра
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unitConfig = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&unitConfig, &adc);
    adc_oneshot_chan_cfg_t channelConfig = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc, POT_CHANNEL, &channelConfig);

    int raw = 0;
    adc_oneshot_read(adc, POT_CHANNEL, &raw);
    smaInit(raw);

    printf("PWM: LED %d Hz (timer 0), motor %d Hz (timer 1), resolution 10 bit (0..%d)\n",
           LED_PWM_FREQ, MOTOR_PWM_FREQ, PWM_MAX_DUTY);

    int counter = 0;
    TickType_t lastWake = xTaskGetTickCount();

    while (1)
    {
        adc_oneshot_read(adc, POT_CHANNEL, &raw);
        uint16_t pot = smaUpdate(raw);

        uint32_t ledDuty = (uint32_t)pot * PWM_MAX_DUTY / ADC_MAX; // яскравість: лінійно 0..100 %
        uint32_t motorDuty = motorDutyFromPot(pot);

        pwmSet(LEDC_CHANNEL_0, ledDuty);
        pwmSet(LEDC_CHANNEL_1, motorDuty);

        if (++counter >= LOG_EVERY)
        {
            counter = 0;
            printf("pot: %4u (%3lu %%) | LED duty: %4lu (%3lu %%) | motor duty: %4lu (%3lu %%)\n",
                   pot, (unsigned long)(pot * 100UL / ADC_MAX),
                   (unsigned long)ledDuty, (unsigned long)(ledDuty * 100 / PWM_MAX_DUTY),
                   (unsigned long)motorDuty, (unsigned long)(motorDuty * 100 / PWM_MAX_DUTY));
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

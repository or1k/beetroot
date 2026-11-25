// ДЗ 3.4: Програвання звуку на зумері (PWM) — ESP-IDF.
// Завдання 1: неблокуючий аудіо-плеєр:
//   - звук через PWM (LEDC): частота ШІМ = частота ноти, заповнення 50 %;
//   - фіксований часовий тік 50 мс від таймера esp_timer;
//   - нічого не блокує: ні FreeRTOS task, ні delay. app_main лише налаштовує все і завершується,
//     далі вся робота — у колбеку таймера.
// Бонус (Завдання 2): кілька мелодій на 7 нотах, вибір командою з Serial Monitor.
// Щоб показати, що програма не заблокована, від того ж тіку незалежно блимає LED.
//
// Схема: GPIO5 -> (+) пасивного зумера, (−) зумера -> GND
//        GPIO4 -> 220 Ом -> LED -> GND

#include <stdio.h>
#include <stdbool.h>
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_timer.h"

#define BUZZER_PIN 5
#define LED_PIN 4

#define TICK_MS 50          // фіксований тік плеєра
#define GAP_TICKS 1         // тиша між нотами (50 мс) — щоб однакові ноти не зливались
#define LED_BLINK_TICKS 10  // LED перемикається кожні 10 тіків = 500 мс

#define PWM_RESOLUTION LEDC_TIMER_10_BIT
#define DUTY_SOUND 512 // 50 % з 1023 — найгучніший звук для зумера
#define DUTY_SILENT 0

// ---------- Ноти (частоти першої октави, Гц) ----------
#define NOTE_C 262
#define NOTE_D 294
#define NOTE_E 330
#define NOTE_F 349
#define NOTE_G 392
#define NOTE_A 440
#define NOTE_B 494
#define REST 0 // пауза

typedef struct
{
    uint16_t freq;  // частота ноти, Гц (0 — пауза)
    uint8_t ticks;  // тривалість у тіках по 50 мс
} Note;

typedef struct
{
    const char *name;
    const Note *notes;
    uint8_t length;
} Melody;

// Тривалості: 4 тіки = 200 мс (коротка), 6 = 300 мс, 12 = 600 мс (довга)
static const Note babyShark[] = {
    {NOTE_C, 4}, {NOTE_D, 4}, {NOTE_E, 6}, {REST, 2},
    {NOTE_C, 4}, {NOTE_D, 4}, {NOTE_E, 6}, {REST, 2},
    {NOTE_C, 4}, {NOTE_D, 4}, {NOTE_E, 12}};

static const Note jingleBells[] = {
    {NOTE_E, 4}, {NOTE_E, 4}, {NOTE_E, 8},
    {NOTE_E, 4}, {NOTE_E, 4}, {NOTE_E, 8},
    {NOTE_E, 4}, {NOTE_G, 4}, {NOTE_C, 6}, {NOTE_D, 2}, {NOTE_E, 12}};

static const Note rockYou[] = {
    {NOTE_C, 4}, {NOTE_C, 4}, {NOTE_D, 6}, {REST, 2},
    {NOTE_C, 4}, {NOTE_C, 4}, {NOTE_D, 6}, {REST, 2},
    {NOTE_C, 4}, {NOTE_C, 4}, {NOTE_D, 4}, {NOTE_E, 12}};

static const Note twinkle[] = {
    {NOTE_C, 6}, {NOTE_C, 6}, {NOTE_G, 6}, {NOTE_G, 6},
    {NOTE_A, 6}, {NOTE_A, 6}, {NOTE_G, 12}};

static const Note macDonald[] = {
    {NOTE_G, 6}, {NOTE_G, 6}, {NOTE_G, 6}, {NOTE_D, 6},
    {NOTE_E, 6}, {NOTE_E, 6}, {NOTE_D, 12}};

// Мелодія з уроку (гілка PFM): там вона грається блокуюче через HAL_Delay(400/800),
// тут — та сама мелодія і ті самі тривалості, але на тіках (8 тіків = 400 мс, 16 = 800 мс)
static const Note twinkleLesson[] = {
    {NOTE_C, 8}, {NOTE_C, 8}, {NOTE_G, 8}, {NOTE_G, 8}, {NOTE_A, 8}, {NOTE_A, 8}, {NOTE_G, 16},
    {NOTE_F, 8}, {NOTE_F, 8}, {NOTE_E, 8}, {NOTE_E, 8}, {NOTE_D, 8}, {NOTE_D, 8}, {NOTE_C, 16}};

#define LEN(a) (sizeof(a) / sizeof(a[0]))

static const Melody melodies[] = {
    {"Baby Shark", babyShark, LEN(babyShark)},
    {"Jingle Bells", jingleBells, LEN(jingleBells)},
    {"We Will Rock You", rockYou, LEN(rockYou)},
    {"Twinkle Twinkle Little Star", twinkle, LEN(twinkle)},
    {"Old MacDonald", macDonald, LEN(macDonald)},
    {"Twinkle Twinkle (full, from lesson)", twinkleLesson, LEN(twinkleLesson)},
};
#define MELODY_COUNT LEN(melodies)

// ---------- Плеєр: машина станів ----------
typedef enum
{
    PLAYER_IDLE, // нічого не грає
    PLAYER_NOTE, // звучить нота
    PLAYER_GAP   // коротка тиша між нотами
} PlayerState;

static PlayerState state = PLAYER_IDLE;
static const Melody *current = NULL;
static uint8_t noteIndex = 0;
static uint8_t ticksLeft = 0;

static void buzzerSet(uint16_t freq)
{
    if (freq == REST)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, DUTY_SILENT);
    }
    else
    {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq); // висота звуку = частота ШІМ
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, DUTY_SOUND);
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void startNote(void)
{
    const Note *note = &current->notes[noteIndex];
    buzzerSet(note->freq);
    ticksLeft = note->ticks;
    state = PLAYER_NOTE;
}

static void playerPlay(uint8_t melodyNumber)
{
    current = &melodies[melodyNumber];
    noteIndex = 0;
    printf("Playing: %s\n", current->name);
    startNote();
}

static void playerStop(void)
{
    buzzerSet(REST);
    state = PLAYER_IDLE;
}

// Викликається кожен тік (50 мс): лише рахує тіки і перемикає стан — нічого не чекає
static void playerTick(void)
{
    if (state == PLAYER_IDLE)
    {
        return;
    }

    if (ticksLeft > 0)
    {
        ticksLeft--;
    }
    if (ticksLeft > 0)
    {
        return; // поточна нота / пауза ще триває
    }

    if (state == PLAYER_NOTE)
    {
        buzzerSet(REST); // нота закінчилась — коротка тиша
        ticksLeft = GAP_TICKS;
        state = PLAYER_GAP;
    }
    else // PLAYER_GAP
    {
        noteIndex++;
        if (noteIndex >= current->length)
        {
            playerStop();
            printf("Done: %s\n", current->name);
        }
        else
        {
            startNote();
        }
    }
}

// ---------- Команди з Serial (без очікування: читаємо лише те, що вже прийшло) ----------
static void handleSerial(void)
{
    uint8_t c;
    while (uart_read_bytes(UART_NUM_0, &c, 1, 0) == 1) // таймаут 0 — не блокує
    {
        if (c >= '1' && c < '1' + MELODY_COUNT)
        {
            playerPlay(c - '1');
        }
        else if (c == 's')
        {
            playerStop();
            printf("Stopped\n");
        }
        else if (c != '\n' && c != '\r')
        {
            printf("Commands: 1..%d - melody, s - stop\n", (int)MELODY_COUNT);
        }
    }
}

// ---------- LED: незалежна "друга задача" на тому ж тіку ----------
static void ledTick(void)
{
    static uint8_t counter = 0;
    static bool on = false;

    if (++counter >= LED_BLINK_TICKS)
    {
        counter = 0;
        on = !on;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, on ? 1023 : 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }
}

// Колбек таймера — кожні 50 мс
static void onTick(void *arg)
{
    handleSerial();
    playerTick();
    ledTick();
}

void app_main(void)
{
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);

    // Зумер: таймер 0, частота змінюється для кожної ноти
    ledc_timer_config_t buzzerTimer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = NOTE_C,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&buzzerTimer);
    ledc_channel_config_t buzzerChannel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = DUTY_SILENT,
    };
    ledc_channel_config(&buzzerChannel);

    // LED: свій таймер 1 (5 кГц), щоб зміна частоти зумера його не чіпала
    ledc_timer_config_t ledTimer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledTimer);
    ledc_channel_config_t ledChannel = {
        .gpio_num = LED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
    };
    ledc_channel_config(&ledChannel);

    // Фіксований тік 50 мс
    esp_timer_handle_t tickTimer;
    esp_timer_create_args_t tickArgs = {
        .callback = onTick,
        .name = "player_tick",
    };
    esp_timer_create(&tickArgs, &tickTimer);
    esp_timer_start_periodic(tickTimer, TICK_MS * 1000);

    printf("Buzzer player. Tick %d ms. Commands:\n", TICK_MS);
    for (int i = 0; i < (int)MELODY_COUNT; i++)
    {
        printf("  %d - %s\n", i + 1, melodies[i].name);
    }
    printf("  s - stop\n");

    playerPlay(0); // одразу програти першу мелодію

    // app_main завершується — програма працює далі лише на таймері
}

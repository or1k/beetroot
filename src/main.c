// ДЗ 2.6: Усунення брязкоту контактів на ESP-IDF (завдання уроку 2.4).
// Усі 4 методи в одній програмі, метод перемикається командою в Serial Monitor:
//   1 — без debounce (кожен interrupt = реакція)
//   2 — time-based: ігноруємо події, якщо з попередньої реакції пройшло < 50 мс
//   3 — state-based на АПАРАТНОМУ ТАЙМЕРІ (як на уроці): interrupt кнопки запускає
//       таймер на 50 мс; коли він спрацює — перевіряємо, чи кнопка досі натиснута
//   4 — polling: interrupt вимкнений, програмний таймер esp_timer кожні 5 мс
//       опитує кнопку і веде машину станів
//   r — обнулити лічильники
//
// Кнопка: GPIO7 -> кнопка -> GND, підтяжка 10 кОм від GPIO7 до 3V3.
// LED: GPIO4 -> 220 Ом -> LED -> GND (перемикається на кожну реакцію).

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"
#include "esp_timer.h"

#define BUTTON_PIN GPIO_NUM_7
#define LED_PIN GPIO_NUM_4

#define DEBOUNCE_MS 50 // методи 2 і 3: вікно антидребезгу
#define POLL_MS 5      // метод 4: період опитування
#define STABLE_MS 20   // метод 4: скільки рівень має бути стабільним

// ---------- Спільні змінні (їх змінюють переривання) — volatile ----------
static volatile uint32_t interruptCount = 0; // всього interrupt (включно з брязкотом)
static volatile bool buttonEvent = false;    // ISR сигналізує "подія"
static volatile int64_t eventTimeUs = 0;     // час першого фронту в пачці

static volatile bool reactionReady = false;  // методи 3 і 4: реакція готова, треба надрукувати
static volatile int64_t reactionFirstUs = 0; // коли почалося натискання
static volatile int64_t reactionTimeUs = 0;  // коли прийнято рішення

static volatile bool timerRunning = false;   // метод 3: таймер антидребезгу вже запущений

static int method = 1;
static uint32_t reactions = 0;
static uint32_t shownCount = 0;
static bool ledOn = false;

static gptimer_handle_t debounceTimer = NULL; // апаратний таймер (метод 3)
static esp_timer_handle_t pollTimer = NULL;   // програмний таймер (метод 4)

// ---------- Переривання кнопки ----------
static void IRAM_ATTR buttonISR(void *arg)
{
    interruptCount++;

    if (!buttonEvent)
    {
        eventTimeUs = esp_timer_get_time();
        buttonEvent = true;
    }

    // Метод 3: перший фронт запускає таймер на DEBOUNCE_MS. Поки таймер іде —
    // решта фронтів (брязкіт) нічого не запускає.
    if (method == 3 && !timerRunning)
    {
        timerRunning = true;
        reactionFirstUs = esp_timer_get_time();
        gptimer_set_raw_count(debounceTimer, 0);
        gptimer_start(debounceTimer);
    }
}

// ---------- Метод 3: таймер антидребезгу спрацював ----------
static bool IRAM_ATTR onDebounceTimer(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *arg)
{
    gptimer_stop(timer);

    // Кнопка досі натиснута -> справжнє натискання. Відпущена -> це був брязкіт або відпускання.
    if (gpio_get_level(BUTTON_PIN) == 0)
    {
        reactionTimeUs = esp_timer_get_time();
        reactionReady = true;
    }

    buttonEvent = false;
    timerRunning = false;
    return false;
}

// ---------- Метод 4: опитування кнопки програмним таймером ----------
typedef enum
{
    RELEASED,       // кнопка відпущена
    MAYBE_PRESSED,  // побачили LOW, чекаємо, чи він стабільний
    PRESSED,        // кнопка натиснута
    MAYBE_RELEASED  // побачили HIGH, чекаємо, чи він стабільний
} PollState;

static void onPollTimer(void *arg)
{
    static PollState state = RELEASED;
    static int64_t stateStartUs = 0;

    int64_t now = esp_timer_get_time();
    bool pressed = gpio_get_level(BUTTON_PIN) == 0;

    switch (state)
    {
    case RELEASED:
        if (pressed)
        {
            state = MAYBE_PRESSED;
            stateStartUs = now;
        }
        break;

    case MAYBE_PRESSED:
        if (!pressed)
        {
            state = RELEASED; // брязкіт або завада
        }
        else if (now - stateStartUs >= STABLE_MS * 1000)
        {
            state = PRESSED;
            reactionFirstUs = stateStartUs;
            reactionTimeUs = now;
            reactionReady = true;
        }
        break;

    case PRESSED:
        if (!pressed)
        {
            state = MAYBE_RELEASED;
            stateStartUs = now;
        }
        break;

    case MAYBE_RELEASED:
        if (pressed)
        {
            state = PRESSED; // брязкіт при відпусканні
        }
        else if (now - stateStartUs >= STABLE_MS * 1000)
        {
            state = RELEASED;
        }
        break;
    }
}

// ---------- Реакція: лічильник, LED, лог ----------
static void react(int64_t firstEdgeUs, int64_t decisionUs)
{
    reactions++;
    ledOn = !ledOn;
    gpio_set_level(LED_PIN, ledOn);
    printf("[method %d] reaction #%lu | interrupts: %lu | delay: %.2f ms\n",
           method, (unsigned long)reactions, (unsigned long)interruptCount,
           (decisionUs - firstEdgeUs) / 1000.0);
}

// ---------- Перемикання методу ----------
static void setMethod(int newMethod)
{
    method = newMethod;
    reactions = 0;
    interruptCount = 0;
    shownCount = 0;
    buttonEvent = false;
    reactionReady = false;

    esp_timer_stop(pollTimer); // помилку "таймер не запущений" просто ігноруємо

    if (method == 4)
    {
        gpio_intr_disable(BUTTON_PIN); // polling — переривань немає зовсім
        esp_timer_start_periodic(pollTimer, POLL_MS * 1000);
    }
    else
    {
        gpio_intr_enable(BUTTON_PIN);
    }

    const char *names[] = {"", "no debounce", "time-based", "state-based (hardware timer)", "polling (esp_timer)"};
    printf("\n=== Method %d: %s ===\n", method, names[method]);
}

static void handleSerial(void)
{
    uint8_t c;
    if (uart_read_bytes(UART_NUM_0, &c, 1, pdMS_TO_TICKS(1)) == 1)
    {
        if (c >= '1' && c <= '4')
        {
            setMethod(c - '0');
        }
        else if (c == 'r')
        {
            setMethod(method);
        }
        else if (c != '\n' && c != '\r')
        {
            printf("commands: 1 / 2 / 3 / 4 (method), r (reset counters)\n");
        }
    }
}

static void init(void)
{
    // UART0 — той самий порт, що й Serial Monitor; драйвер потрібен, щоб читати команди
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);

    // LED
    gpio_config_t led = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led);
    gpio_set_level(LED_PIN, 0);

    // Кнопка: лише зовнішня підтяжка 10 кОм, interrupt на спадаючий фронт (натискання)
    gpio_config_t btn = {
        .pin_bit_mask = 1ULL << BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, buttonISR, NULL);

    // Апаратний таймер для методу 3: 1 тік = 1 мкс, один раз спрацьовує через DEBOUNCE_MS
    gptimer_config_t timerConfig = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    gptimer_new_timer(&timerConfig, &debounceTimer);

    gptimer_event_callbacks_t callbacks = {.on_alarm = onDebounceTimer};
    gptimer_register_event_callbacks(debounceTimer, &callbacks, NULL);
    gptimer_enable(debounceTimer);

    gptimer_alarm_config_t alarm = {
        .alarm_count = DEBOUNCE_MS * 1000,
        .flags.auto_reload_on_alarm = false,
    };
    gptimer_set_alarm_action(debounceTimer, &alarm);

    // Програмний таймер для методу 4 (запускається лише в методі 4)
    esp_timer_create_args_t pollArgs = {
        .callback = onPollTimer,
        .name = "button_poll",
    };
    esp_timer_create(&pollArgs, &pollTimer);
}

void app_main(void)
{
    init();
    printf("Debounce test (ESP-IDF). Commands: 1 / 2 / 3 / 4 (method), r (reset counters)\n");
    setMethod(1);

    static int64_t lastReactionUs = 0; // метод 2

    while (1) // superloop
    {
        handleSerial(); // чекає символ не довше 1 мс

        if (method == 1)
        {
            // Без debounce: кожен interrupt — реакція, просто виводимо лічильник
            if (interruptCount != shownCount)
            {
                shownCount = interruptCount;
                reactions = shownCount;
                buttonEvent = false;
                printf("[method 1] interrupt counter: %lu\n", (unsigned long)shownCount);
            }
        }
        else if (method == 2)
        {
            // Time-based поза ISR: ігноруємо події ближче ніж DEBOUNCE_MS до попередньої реакції
            if (buttonEvent)
            {
                buttonEvent = false;
                int64_t now = esp_timer_get_time();
                if (now - lastReactionUs >= DEBOUNCE_MS * 1000)
                {
                    lastReactionUs = now;
                    react(eventTimeUs, now);
                }
            }
        }
        else if (reactionReady)
        {
            // Методи 3 і 4: рішення вже прийняв таймер, тут лише лог і LED
            reactionReady = false;
            react(reactionFirstUs, reactionTimeUs);
        }
    }
}

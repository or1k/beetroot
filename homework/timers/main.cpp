#include <Arduino.h>
#include <esp_task_wdt.h>

// ДЗ 2.5: Таймери. Витяжний вентилятор вмикається кожну годину на 15 хвилин.
// Цикл повністю веде апаратний таймер: його переривання раз на TICK_MS
// саме вмикає і вимикає вентилятор. loop() лише друкує лог і скидає watchdog.
//
// Вентилятор = модуль реле (через транзистор 2N2222, як у ДЗ з реле) на GPIO17.
// LED на GPIO4 горить, коли вентилятор працює; LED на GPIO5 — коли вимкнений.

#define FAN_PIN 17
#define LED_ON_PIN 4  // зелений: вентилятор працює
#define LED_OFF_PIN 5 // червоний: вентилятор вимкнений

// 1 — режим налагодження: "година" = 8 с, "15 хвилин" = 2 с (та сама пропорція 1/4)
#define DEBUG_MODE 1

#if DEBUG_MODE
#define FAN_PERIOD_MS 8000UL // як часто вмикати
#define FAN_ON_MS 2000UL     // скільки працювати
#else
#define FAN_PERIOD_MS (60UL * 60UL * 1000UL) // 1 година
#define FAN_ON_MS (15UL * 60UL * 1000UL)     // 15 хвилин
#endif

#define TICK_MS 100UL        // таймер спрацьовує кожні 100 мс
#define WDT_TIMEOUT_S 5      // якщо loop() зависне на 5 с — плата перезавантажиться

// Захист від помилки в налаштуваннях: час роботи має бути меншим за період
static_assert(FAN_ON_MS < FAN_PERIOD_MS, "FAN_ON_MS must be less than FAN_PERIOD_MS");

constexpr u32_t PERIOD_TICKS = FAN_PERIOD_MS / TICK_MS;
constexpr u32_t ON_TICKS = FAN_ON_MS / TICK_MS;

hw_timer_t *timer = nullptr;

// Змінні, які змінює переривання таймера — volatile
volatile u32_t tick = 0;           // номер тіку всередині періоду: 0 .. PERIOD_TICKS-1
volatile bool fanOn = false;
volatile bool stateChanged = false; // прапорець для логу в loop()
volatile u32_t blockedStarts = 0;   // скільки разів захист не дав запустити вдруге

// Вмикаємо/вимикаємо виходи через регістри (як на уроці про бітові операції) — швидко і безпечно в ISR
void IRAM_ATTR setFan(bool on)
{
  if (on)
  {
    GPIO.out_w1ts = (1UL << FAN_PIN) | (1UL << LED_ON_PIN); // встановити біти — HIGH
    GPIO.out_w1tc = (1UL << LED_OFF_PIN);                   // скинути біт — LOW
  }
  else
  {
    GPIO.out_w1tc = (1UL << FAN_PIN) | (1UL << LED_ON_PIN);
    GPIO.out_w1ts = (1UL << LED_OFF_PIN);
  }
  fanOn = on;
  stateChanged = true;
}

// Переривання таймера кожні TICK_MS. Саме воно веде весь цикл вентилятора.
void IRAM_ATTR onTimer()
{
  if (tick == 0)
  {
    if (fanOn)
    {
      blockedStarts++; // захист від повторного запуску: вентилятор уже працює
    }
    else
    {
      setFan(true);
    }
  }
  else if (tick == ON_TICKS && fanOn)
  {
    setFan(false);
  }

  tick++;
  if (tick >= PERIOD_TICKS)
  {
    tick = 0; // новий період — цикл повторюється сам
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(LED_ON_PIN, OUTPUT);
  pinMode(LED_OFF_PIN, OUTPUT);
  setFan(false);

  // Watchdog: якщо loop() перестане його скидати — перезавантаження
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  // Апаратний таймер: дільник 80 -> 1 МГц (1 тік = 1 мкс), переривання кожні TICK_MS
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TICK_MS * 1000, true); // true — автоповтор
  timerAlarmEnable(timer);

  Serial.printf("Fan timer: period %lu ms, on %lu ms (%s)\n",
                FAN_PERIOD_MS, FAN_ON_MS, DEBUG_MODE ? "debug" : "real");
}

void loop()
{
  // loop() НЕ керує вентилятором — лише логує стан і скидає watchdog
  if (stateChanged)
  {
    stateChanged = false;
    Serial.printf("[%8lu ms] FAN %s\n", millis(), fanOn ? "ON" : "OFF");
  }

  if (blockedStarts > 0)
  {
    Serial.printf("Restart blocked %u time(s): fan was still running\n", blockedStarts);
    blockedStarts = 0;
  }

  esp_task_wdt_reset();
}

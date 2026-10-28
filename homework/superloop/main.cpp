#include <Arduino.h>

// ДЗ 2.3: Superloop та незалежні задачі.
// Три світлодіоди блимають кожен зі своїм інтервалом в одному loop():
//   LED1 — кожні 200 мс, LED2 — кожні 500 мс, LED3 — кожні 1000 мс.
// Без delay() і без while для очікування: кожен LED сам перевіряє millis().

#define LED1_PIN 4
#define LED2_PIN 5
#define LED3_PIN 6

constexpr u32_t LED1_INTERVAL_MS = 200;
constexpr u32_t LED2_INTERVAL_MS = 500;
constexpr u32_t LED3_INTERVAL_MS = 1000;
constexpr u32_t REPORT_INTERVAL_MS = 1000; // як часто друкувати статистику в Serial

// Світлодіод, який сам знає свій пін, інтервал і коли перемикався востаннє
class BlinkingLed
{
private:
  u8_t pin;
  u32_t interval;
  u32_t lastToggleTime = 0;
  bool isOn = false;
  u32_t toggles = 0; // скільки разів перемкнувся (для перевірки в Serial)

public:
  BlinkingLed(u8_t ledPin, u32_t intervalMs)
  {
    pin = ledPin;
    interval = intervalMs;
  }

  void init()
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  // Викликається в кожному проході loop(). Нічого не чекає: якщо час не настав — одразу виходить.
  void update(u32_t now)
  {
    if (now - lastToggleTime >= interval)
    {
      lastToggleTime = now;
      isOn = !isOn;
      digitalWrite(pin, isOn ? HIGH : LOW);
      toggles++;
    }
  }

  // Повертає кількість перемикань з минулого виклику і обнуляє лічильник
  u32_t takeToggles()
  {
    u32_t count = toggles;
    toggles = 0;
    return count;
  }
};

BlinkingLed led1(LED1_PIN, LED1_INTERVAL_MS);
BlinkingLed led2(LED2_PIN, LED2_INTERVAL_MS);
BlinkingLed led3(LED3_PIN, LED3_INTERVAL_MS);

void setup()
{
  Serial.begin(115200);
  led1.init();
  led2.init();
  led3.init();
  Serial.println("Superloop: LED1 200 ms | LED2 500 ms | LED3 1000 ms");
}

// Звіт раз на секунду: скільки разів перемкнувся кожен LED.
// Очікується 5 / 2 / 1 — це доводить, що задачі працюють незалежно.
void report(u32_t now)
{
  static u32_t lastReportTime = 0;
  static u32_t loopCount = 0;

  loopCount++;

  if (now - lastReportTime >= REPORT_INTERVAL_MS)
  {
    lastReportTime = now;
    Serial.printf("t=%5u ms | LED1: %u | LED2: %u | LED3: %u toggles | loop ran %u times\n",
                  now, led1.takeToggles(), led2.takeToggles(), led3.takeToggles(), loopCount);
    loopCount = 0;
  }
}

void loop() // superloop: кожна задача робить короткий крок і одразу повертає керування
{
  u32_t now = millis();

  led1.update(now);
  led2.update(now);
  led3.update(now);

  report(now);
}

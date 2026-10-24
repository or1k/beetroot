#include <Arduino.h>

// ДЗ: вимірювання часу спрацювання реле.
// GPIO17 керує обмоткою реле (модуль реле або транзистор).
// GPIO18 читає «сухий» контакт реле: COM -> GND, NO -> GPIO18, режим INPUT_PULLUP.
//   Контакт розімкнений -> на піні HIGH, замкнений -> LOW.
// Програма 10 разів вмикає і вимикає реле і міряє, через скільки контакт реально перемкнувся.

#define RELAY_PIN 17
#define CONTACT_PIN 18

// Модуль реле (живлення 5 В) вмикається, коли IN притиснутий до землі, і вимикається, лише коли IN вільний.
// 3.3 В з піна ESP його не вимикають, тому IN керується через транзистор 2N2222:
//   GPIO17 -> 220 Ом -> база; 10 кОм база -> GND; колектор -> IN; емітер -> GND.
//   HIGH на GPIO17 -> транзистор відкритий -> IN на землі -> реле увімкнене.
//   LOW  на GPIO17 -> транзистор закритий -> IN вільний -> реле вимкнене.
#define RELAY_ON HIGH
#define RELAY_OFF LOW

constexpr u8_t MEASUREMENTS = 10;      // скільки разів вмикати/вимикати
constexpr u32_t PAUSE_MS = 500;        // пауза після перемикання: брязкіт закінчився, реле заспокоїлось
constexpr u32_t TIMEOUT_MS = 200;      // якщо контакт не перемкнувся за цей час — помилка підключення

// Змінні, які змінює переривання — volatile
volatile bool edgeCaught = false;  // контакт уже перемкнувся (перший фронт)
volatile u32_t edgeTimeUs = 0;     // час першого фронту
volatile u32_t edgeCount = 0;      // скільки всього фронтів (1 = без брязкоту)

enum State
{
  START_ON,   // подати струм на обмотку
  WAIT_ON,    // чекати, поки контакт замкнеться
  PAUSE_ON,   // пауза, поки йде брязкіт
  START_OFF,  // зняти струм з обмотки
  WAIT_OFF,   // чекати, поки контакт розімкнеться
  PAUSE_OFF,
  DONE
};

State state = START_ON;
u32_t commandTimeUs = 0; // коли подали команду на реле
u32_t stateTimeMs = 0;   // коли почався поточний стан
u8_t measurement = 0;    // номер вимірювання
u32_t sumOnUs = 0;
u32_t sumOffUs = 0;

// ISR: лише запам'ятовуємо час ПЕРШОГО фронту. Наступні фронти — це брязкіт, їх тільки рахуємо.
void IRAM_ATTR contactISR()
{
  edgeCount++;
  if (!edgeCaught)
  {
    edgeTimeUs = micros();
    edgeCaught = true;
  }
}

// Подати команду на реле і почати вимірювання
void switchRelay(int level)
{
  edgeCaught = false;
  edgeCount = 0;
  commandTimeUs = micros();
  digitalWrite(RELAY_PIN, level);
  stateTimeMs = millis();
}

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(CONTACT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), contactISR, CHANGE);

  delay(1000); // лише в setup: час відкрити Serial Monitor
  Serial.println("Relay timing test");
  Serial.printf("CONTACT_PIN at start: %s (relay is OFF, expected HIGH)\n",
                digitalRead(CONTACT_PIN) == LOW ? "LOW" : "HIGH");
  Serial.println(" #  | ON, ms  | edges | OFF, ms | edges");
}

void loop()
{
  u32_t now = millis();

  switch (state)
  {
  case START_ON:
    switchRelay(RELAY_ON);
    state = WAIT_ON;
    break;

  case WAIT_ON:
    if (edgeCaught)
    {
      sumOnUs += edgeTimeUs - commandTimeUs;
      state = PAUSE_ON;
    }
    else if (now - stateTimeMs > TIMEOUT_MS)
    {
      Serial.println("ERROR: contact did not close - check wiring");
      Serial.printf("CONTACT_PIN now: %s (relay is ON, expected LOW)\n",
                    digitalRead(CONTACT_PIN) == LOW ? "LOW" : "HIGH");
      state = DONE;
    }
    break;

  case PAUSE_ON:
    if (now - stateTimeMs > PAUSE_MS)
    {
      Serial.printf("%2u  | %7.3f | %5u | ", measurement + 1, (edgeTimeUs - commandTimeUs) / 1000.0, edgeCount);
      state = START_OFF;
    }
    break;

  case START_OFF:
    switchRelay(RELAY_OFF);
    state = WAIT_OFF;
    break;

  case WAIT_OFF:
    if (edgeCaught)
    {
      sumOffUs += edgeTimeUs - commandTimeUs;
      state = PAUSE_OFF;
    }
    else if (now - stateTimeMs > TIMEOUT_MS)
    {
      Serial.println("\nERROR: contact did not open - check wiring");
      state = DONE;
    }
    break;

  case PAUSE_OFF:
    if (now - stateTimeMs > PAUSE_MS)
    {
      Serial.printf("%7.3f | %5u\n", (edgeTimeUs - commandTimeUs) / 1000.0, edgeCount);
      measurement++;

      if (measurement >= MEASUREMENTS)
      {
        Serial.printf("\nAverage ON:  %.3f ms\n", sumOnUs / 1000.0 / MEASUREMENTS);
        Serial.printf("Average OFF: %.3f ms\n", sumOffUs / 1000.0 / MEASUREMENTS);
        state = DONE;
      }
      else
      {
        state = START_ON;
      }
    }
    break;

  case DONE:
  {
    // Вимірювання завершено (RST — повторити). Поки що показуємо кожну зміну на CONTACT_PIN,
    // щоб можна було перевірити проводи руками.
    static int lastLevel = -1;
    int level = digitalRead(CONTACT_PIN);
    if (level != lastLevel)
    {
      lastLevel = level;
      Serial.printf("CONTACT_PIN: %s\n", level == LOW ? "LOW" : "HIGH");
    }
    break;
  }
  }
}

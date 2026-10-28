#include <Arduino.h>

// ДЗ 2.4: Усунення брязкоту контактів кнопки.
// Усі 4 методи в одній програмі. Метод перемикається командою в Serial Monitor:
//   1 — без debounce (кожен interrupt = реакція)
//   2 — time-based: ігноруємо події, якщо з попередньої реакції пройшло < 50 мс
//   3 — state-based: приймаємо подію, лише якщо кнопка досі натиснута
//   4 — polling: без переривань, опитування кожні 5 мс + машина станів
//   r — обнулити лічильники
// На кожну реакцію LED перемикається і в лог пишеться номер реакції та затримка.
//
// Кнопка: GPIO7 -> кнопка -> GND, підтяжка 10 кОм від GPIO7 до 3V3.

#define BUTTON_PIN 7
#define LED_PIN 4

constexpr u32_t DEBOUNCE_MS = 50; // метод 2: мінімальний час між реакціями
constexpr u32_t CONFIRM_MS = 10;  // метод 3: через скільки після події перевіряти, чи кнопка ще натиснута
constexpr u32_t RELEASE_MS = 50;  // метод 3: скільки кнопка має бути відпущена, щоб прийняти нове натискання
constexpr u32_t POLL_MS = 5;      // метод 4: період опитування
constexpr u32_t STABLE_MS = 20;   // метод 4: скільки рівень має бути стабільним

// ---------- Змінні переривання ----------
volatile u32_t interruptCount = 0;  // всього interrupt (включно з брязкотом)
volatile bool buttonEvent = false;  // ISR лише сигналізує "подія"
volatile u32_t eventTimeUs = 0;     // час першого interrupt у пачці (для затримки)

void IRAM_ATTR buttonISR()
{
  interruptCount++;
  if (!buttonEvent)
  {
    eventTimeUs = micros();
    buttonEvent = true;
  }
}

// ---------- Загальне ----------
u8_t method = 1;
u32_t reactions = 0; // скільки разів програма "зреагувала" на кнопку
bool ledOn = false;

// Реакція на натискання: лічильник, LED, лог із затримкою від першого фронту
void react(u32_t firstEdgeUs)
{
  reactions++;
  ledOn = !ledOn;
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  Serial.printf("[method %u] reaction #%u | interrupts: %u | delay: %.2f ms\n",
                method, reactions, interruptCount, (micros() - firstEdgeUs) / 1000.0);
}

// ---------- Метод 1: без debounce ----------
// Кожен interrupt = реакція. Просто виводимо лічильник, щойно він змінився.
u32_t shownCount = 0;

void methodNoDebounce()
{
  if (interruptCount != shownCount)
  {
    shownCount = interruptCount;
    reactions = shownCount;
    buttonEvent = false;
    Serial.printf("[method 1] interrupt counter: %u\n", shownCount);
  }
}

// ---------- Метод 2: time-based (поза ISR) ----------
void methodTimeBased(u32_t now)
{
  static u32_t lastReactionTime = 0;

  if (buttonEvent)
  {
    buttonEvent = false;
    if (now - lastReactionTime >= DEBOUNCE_MS)
    {
      lastReactionTime = now;
      react(eventTimeUs);
    }
  }
}

// ---------- Метод 3: state-based ----------
void methodStateBased(u32_t now)
{
  static bool waitingConfirm = false; // подія прийшла, чекаємо CONFIRM_MS
  static u32_t confirmStart = 0;
  static bool armed = true;           // кнопку було відпущено — можна приймати нове натискання
  static u32_t lastTimePressed = 0;

  bool pressed = digitalRead(BUTTON_PIN) == LOW;

  // Нове натискання приймаємо лише після того, як кнопка була відпущена RELEASE_MS
  if (pressed)
  {
    lastTimePressed = now;
  }
  else if (!armed && now - lastTimePressed >= RELEASE_MS)
  {
    armed = true;
  }

  if (buttonEvent && !waitingConfirm)
  {
    buttonEvent = false;
    waitingConfirm = true;
    confirmStart = now;
  }

  if (waitingConfirm && now - confirmStart >= CONFIRM_MS)
  {
    waitingConfirm = false;
    buttonEvent = false; // брязкіт за час очікування нас не цікавить

    // Кнопка досі натиснута -> справжнє натискання. Відпущена -> це брязкіт при відпусканні.
    if (pressed && armed)
    {
      armed = false;
      react(eventTimeUs);
    }
  }
}

// ---------- Метод 4: polling + машина станів (без interrupt) ----------
enum PollState
{
  RELEASED,       // кнопка відпущена
  MAYBE_PRESSED,  // побачили LOW, чекаємо, чи він стабільний
  PRESSED,        // кнопка натиснута
  MAYBE_RELEASED  // побачили HIGH, чекаємо, чи він стабільний
};

void methodPolling(u32_t now)
{
  static PollState state = RELEASED;
  static u32_t lastPollTime = 0;
  static u32_t stateStart = 0;
  static u32_t firstLowUs = 0;

  if (now - lastPollTime < POLL_MS)
  {
    return;
  }
  lastPollTime = now;

  bool pressed = digitalRead(BUTTON_PIN) == LOW;

  switch (state)
  {
  case RELEASED:
    if (pressed)
    {
      state = MAYBE_PRESSED;
      stateStart = now;
      firstLowUs = micros();
    }
    break;

  case MAYBE_PRESSED:
    if (!pressed)
    {
      state = RELEASED; // це був брязкіт або завада
    }
    else if (now - stateStart >= STABLE_MS)
    {
      state = PRESSED;
      react(firstLowUs);
    }
    break;

  case PRESSED:
    if (!pressed)
    {
      state = MAYBE_RELEASED;
      stateStart = now;
    }
    break;

  case MAYBE_RELEASED:
    if (pressed)
    {
      state = PRESSED; // брязкіт при відпусканні
    }
    else if (now - stateStart >= STABLE_MS)
    {
      state = RELEASED;
    }
    break;
  }
}

// ---------- Команди з Serial ----------
void setMethod(u8_t newMethod)
{
  method = newMethod;
  reactions = 0;
  interruptCount = 0;
  shownCount = 0;
  buttonEvent = false;

  if (method == 4)
  {
    detachInterrupt(digitalPinToInterrupt(BUTTON_PIN)); // polling — переривань немає зовсім
  }
  else
  {
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  }

  const char *names[] = {"", "no debounce", "time-based", "state-based", "polling"};
  Serial.printf("\n=== Method %u: %s ===\n", method, names[method]);
}

void handleSerial()
{
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "1" || cmd == "2" || cmd == "3" || cmd == "4")
    {
      setMethod(cmd.toInt());
    }
    else if (cmd == "r")
    {
      setMethod(method);
    }
    else
    {
      Serial.println("commands: 1 / 2 / 3 / 4 (method), r (reset counters)");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  // Тільки зовнішня підтяжка 10 кОм (за завданням). Внутрішню INPUT_PULLUP не вмикаємо:
  // з RC-фільтром вона утворює дільник із послідовним резистором і "підвішує" рівень на піні.
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("Debounce test. Commands: 1 / 2 / 3 / 4 (method), r (reset counters)");
  setMethod(1);
}

void loop()
{
  u32_t now = millis();

  handleSerial();

  if (method == 1)
  {
    methodNoDebounce();
  }
  else if (method == 2)
  {
    methodTimeBased(now);
  }
  else if (method == 3)
  {
    methodStateBased(now);
  }
  else
  {
    methodPolling(now);
  }
}

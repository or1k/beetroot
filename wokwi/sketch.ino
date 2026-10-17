// Wokwi: all project files merged into one sketch (generated from src/ + include/)
#include <Arduino.h>

// ===== include\config.h =====

// ---------- Піни ----------
#define HEADLIGHT_LED 18 // білий LED — «ближнє світло» (Урок 1.2)
#define HAZARD_LED 16    // помаранчевий LED — аварійка (Урок 1.3)
#define LDR 2            // фоторезистор, ADC1_CH1 (Урок 1.6)
#define BTN 17           // кнопка/датчик удару між GPIO17 і GND, внутрішня підтяжка (Урок 1.4, 1.5)

// ---------- Таймінги ----------
#define LDR_PERIOD_MS 10      // період опитування LDR
#define BTN_PERIOD_MS 5       // період опитування кнопки
#define BTN_LONG_PRESS_MS 1000 // коротше — шум/завал, довше — реальне затискання
#define HAZARD_BLINK_MS 333   // півперіод блимання: ~1.5 Гц, як у реальної аварійки
#define LOG_PERIOD_MS 500     // як часто друкувати стан у Serial

// ---------- LDR ----------
// Гістерезис, щоб фара не мерехтіла на межі освітленості
#define LDR_DARK_THRESHOLD 1800  // нижче — темно, вмикаємо світло
#define LDR_LIGHT_THRESHOLD 2200 // вище — світло, вимикаємо

// 0 — наша схема (3.3V -> LDR -> пін -> 10k -> GND): темно = мала напруга.
// 1 — модуль Wokwi (10k -> пін -> LDR -> GND): темно = велика напруга, інвертуємо.
#define LDR_INVERTED 1

#define N 32 // розмір вікна ковзного середнього

// ===== include\Led.h =====


class Led
{
private:
    u8_t pin;
    bool state = false;
    u32_t lastToggle = 0;

public:
    Led(u8_t pin) : pin(pin) {}
    void init();
    void handler(bool light_on);    // просто увімкнути / вимкнути
    void blink(u32_t halfPeriodMs); // неблокуюче блимання, викликати в кожному loop()
};

// ===== include\Button.h =====

class Button
{
private:
    u32_t lastTimeChangeBtn = 0;
    u32_t btnTimekeeper = 0;  // момент натискання (0 — кнопка відпущена)
    bool longPressFired = false; // подія вже спрацювала для цього затискання
    bool longPressEvent = false; // непрочитана подія довгого натискання

public:
    void init();
    void poll();
    bool get_long_press(); // true один раз на кожне довге затискання
};

// ===== include\Ldr.h =====


class Ldr
{
private:
    u16_t readings[N];
    u8_t  idx = 0;
    u32_t sum = 0;
    u16_t filtered = 0;
    bool ldrState = false; // true — темно
    u32_t lastTimeChangeLdr = 0;

    u16_t movingAvg(u16_t rawData);
    void filterInit(u16_t initData);

public:
    void init();
    void poll();
    bool get_state();   // true — темно (тунель), треба вмикати світло
    u16_t get_value();  // відфільтроване значення АЦП
};

// ===== src\Led.cpp =====

void Led::init()
{
    pinMode(pin, OUTPUT); // налаштовуємо пін на ВИХІД (led)
}

void Led::handler(bool light_on)
{
    state = light_on;
    digitalWrite(pin, state ? HIGH : LOW);
}

void Led::blink(u32_t halfPeriodMs)
{
    // Меандр 50/50: перемикаємо стан кожні halfPeriodMs без delay()
    if (millis() - lastToggle >= halfPeriodMs)
    {
        lastToggle = millis();
        handler(!state);
    }
}

// ===== src\Button.cpp =====

void Button::init()
{
    // Внутрішня підтяжка до 3.3 В: у спокої HIGH, натиснуто (замкнуто на GND) — LOW
    pinMode(BTN, INPUT_PULLUP);
}

void Button::poll()
{
    if (millis() - lastTimeChangeBtn > BTN_PERIOD_MS) // періодична перевірка стану кнопки
    {
        lastTimeChangeBtn = millis();
        bool pressed = digitalRead(BTN) == LOW;

        if (pressed)
        {
            if (btnTimekeeper == 0)
            {
                btnTimekeeper = millis(); // натискання щойно почалось — запам'ятовуємо час
            }
            else if (!longPressFired && millis() - btnTimekeeper > BTN_LONG_PRESS_MS)
            {
                longPressFired = true; // реальне тривале затискання — одна подія
                longPressEvent = true;
                Serial.println("BTN: long press -> event");
            }
        }
        else if (btnTimekeeper != 0)
        {
            // Кнопку відпущено — друкуємо тривалість, щоб звірити з логічним аналізатором
            u32_t duration = millis() - btnTimekeeper;
            Serial.printf("BTN: released after %u ms (%s)\n", duration,
                          longPressFired ? "real press" : "short / noise, ignored");
            btnTimekeeper = 0;
            longPressFired = false;
        }
    }
}

bool Button::get_long_press()
{
    bool event = longPressEvent;
    longPressEvent = false;
    return event;
}

// ===== src\Ldr.cpp =====

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

// ===== src\main.cpp =====

// Варіант 2. Адаптивна фара та аварійна сигналізація
//  - LDR на «лобовому склі»: темно (тунель) -> вмикається ближнє світло (білий LED)
//  - Кнопка = датчик удару: коротке спрацювання — шум, ігноруємо;
//    тривале затискання -> вмикає/вимикає аварійку (помаранчевий LED блимає ~1.5 Гц)

Ldr ldr;
Button button;
Led headlight(HEADLIGHT_LED);
Led hazard(HAZARD_LED);

bool hazardOn = false;
u32_t lastLog = 0;

void setup()
{
  Serial.begin(115200);
  ldr.init();
  button.init();
  headlight.init();
  hazard.init();
  Serial.println("setuped\n");
}

void loop()
{
  ldr.poll();
  button.poll();

  // Адаптивне світло
  headlight.handler(ldr.get_state());

  // Аварійка: кожне реальне затискання перемикає режим
  if (button.get_long_press())
  {
    hazardOn = !hazardOn;
    Serial.printf("HAZARD: %s\n", hazardOn ? "ON" : "OFF");
  }

  if (hazardOn)
  {
    hazard.blink(HAZARD_BLINK_MS);
  }
  else
  {
    hazard.handler(false);
  }

  // Стан системи раз на LOG_PERIOD_MS
  if (millis() - lastLog > LOG_PERIOD_MS)
  {
    lastLog = millis();
    Serial.printf("LDR filtered: %4u | headlight: %s | hazard: %s\n",
                  ldr.get_value(), ldr.get_state() ? "ON " : "OFF", hazardOn ? "ON" : "OFF");
  }
}

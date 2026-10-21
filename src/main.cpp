#include <Arduino.h>
#include "Config.h"
#include "Led.h"

// Режими роботи LED, які перемикає кнопка
enum class LedMode
{
    Blink,     // блимання
    AlwaysOn,  // постійно увімкнено
    AlwaysOff  // постійно вимкнено
};

// Глобальні змінні — лише необхідні для стану та переривання
Led led(Config::LED_PIN);
LedMode mode = LedMode::Blink;
volatile bool buttonPressed = false; // volatile — бо змінюється в перериванні (ISR)

// Обробник переривання: мінімальний — лише ставимо прапорець
void IRAM_ATTR buttonISR()
{
    buttonPressed = true;
}

// Перемкнути режим по колу: Блимання -> Увімкнено -> Вимкнено -> Блимання
void nextMode()
{
    if (mode == LedMode::Blink)
    {
        mode = LedMode::AlwaysOn;
        led.set(LedState::On);
        Serial.println("Mode: Always ON");
    }
    else if (mode == LedMode::AlwaysOn)
    {
        mode = LedMode::AlwaysOff;
        led.set(LedState::Off);
        Serial.println("Mode: Always OFF");
    }
    else
    {
        mode = LedMode::Blink;
        Serial.println("Mode: Blink");
    }
}

// Обробка кнопки з антидребезгом
void handleButton()
{
    // static — змінна зберігає значення між викликами функції
    static u32_t lastTimeButtonDown = 0; // коли кнопка востаннє була натиснута (LOW)

    u32_t now = millis();

    if (buttonPressed)
    {
        buttonPressed = false;

        // Натискання справжнє, лише якщо перед ним кнопка була відпущена довше за DEBOUNCE_MS.
        // Брязкіт при натисканні і при відпусканні йде, поки кнопка ще "натиснута", тому відсікається.
        if (now - lastTimeButtonDown > Config::DEBOUNCE_MS)
        {
            nextMode();
        }
    }

    if (digitalRead(Config::BUTTON_PIN) == LOW)
    {
        lastTimeButtonDown = now;
    }
}

// Неблокуюче блимання без delay()
void handleBlink()
{
    static u32_t lastBlinkTime = 0;

    if (mode == LedMode::Blink && millis() - lastBlinkTime >= Config::BLINK_INTERVAL_MS)
    {
        lastBlinkTime = millis();
        led.toggle();
    }
}

// Вимірювання часу однієї ітерації loop()
void measureLoopTime(u32_t loopStartUs)
{
    static u32_t loopCount = 0;
    static u32_t totalTimeUs = 0;

    totalTimeUs += micros() - loopStartUs;
    loopCount++;

    if (loopCount >= Config::LOOPS_TO_MEASURE)
    {
        Serial.printf("Average loop time: %u us\n", totalTimeUs / loopCount);
        loopCount = 0;
        totalTimeUs = 0;
    }
}

void setup()
{
    Serial.begin(Config::SERIAL_SPEED);
    led.init();

    pinMode(Config::BUTTON_PIN, INPUT_PULLUP); // відпущена = HIGH, натиснута = LOW
    attachInterrupt(digitalPinToInterrupt(Config::BUTTON_PIN), buttonISR, FALLING);

    Serial.println("Ready. Mode: Blink");
}

void loop() // superloop: кожна функція робить короткий крок і не блокує інші
{
    u32_t loopStartUs = micros();

    handleButton();
    handleBlink();

    measureLoopTime(loopStartUs);
}

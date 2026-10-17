#include <Arduino.h>

// Проєкт: Поліцейська мигалка.
// Червоний і синій світлодіоди блимають по черзі серіями по 3 спалахи.
// Кнопка на GPIO18 перемикає швидкість мигалки.

#define LED_B 16       // синій світлодіод (через резистор 220 Ом на GND)
#define LED_R 17       // червоний світлодіод (через резистор 220 Ом на GND)
#define BUTTON_SPEED 18 // кнопка між GPIO18 і GND

// Тривалість одного спалаху для кожної швидкості (мс)
const u32_t SPEEDS[] = {120, 70, 40};
const u8_t SPEEDS_COUNT = 3;
u8_t speedIndex = 0;

u8_t lastButton = HIGH;

// Перевірити кнопку: при натисканні перейти до наступної швидкості.
void checkButton() {
    u8_t button = digitalRead(BUTTON_SPEED);
    if (button == LOW && lastButton == HIGH) {
        speedIndex = (speedIndex + 1) % SPEEDS_COUNT;
        Serial.printf("Швидкість: %u мс\n", SPEEDS[speedIndex]);
    }
    lastButton = button;
}

// Очікування, під час якого кнопка продовжує опитуватися.
void wait(u32_t ms) {
    u32_t start = millis();
    while (millis() - start < ms) {
        checkButton();
        delay(5);  // заодно захист від брязкоту контактів
    }
}

// Блимнути світлодіодом задану кількість разів.
void flash(u8_t pin, u8_t times) {
    for (u8_t i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        wait(SPEEDS[speedIndex]);
        digitalWrite(pin, LOW);
        wait(SPEEDS[speedIndex]);
    }
}

void setup() {
    pinMode(LED_B, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(BUTTON_SPEED, INPUT_PULLUP);
    Serial.begin(115200);
}

void loop() {
    flash(LED_R, 3);                  // червоний: три спалахи
    wait(SPEEDS[speedIndex] * 2);     // коротка пауза
    flash(LED_B, 3);                  // синій: три спалахи
    wait(SPEEDS[speedIndex] * 2);
}

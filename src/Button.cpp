#include "Button.h"
#include "config.h"
#include <Arduino.h>

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

#include <Arduino.h>
#include "config.h"
#include "Button.h"
#include "Ldr.h"
#include "Led.h"

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

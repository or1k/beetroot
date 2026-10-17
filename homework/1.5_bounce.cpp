#include <Arduino.h>

// ДЗ 1.5: дослідження брязкоту контактів кнопки.
// Кнопка між GPIO16 і GND (pull-up), канал 1 логічного аналізатора — на GPIO16.
// Крім загального лічильника, рахуємо імпульси та тривалість брязкоту
// для кожного натискання окремо — для порівняння з логічним аналізатором.

#define BUTTON_IN 16  // Кнопка підключена до GPIO 16

// Натискання вважається завершеним, якщо фронтів не було QUIET_MS мілісекунд
const uint32_t QUIET_MS = 100;

volatile uint32_t button_counter = 0;   // всього FALLING-фронтів
volatile uint32_t first_edge_us = 0;    // час першого фронту в поточному натисканні
volatile uint32_t last_edge_us = 0;     // час останнього фронту
volatile uint32_t press_edges = 0;      // фронтів у поточному натисканні

uint32_t press_number = 0;

// Швидка функція обробки переривань (ISR), розміщується в IRAM
void IRAM_ATTR button_isr() {
  uint32_t now = micros();
  if (press_edges == 0) first_edge_us = now;
  last_edge_us = now;
  press_edges++;
  button_counter++;
}

void setup() {
  pinMode(BUTTON_IN, INPUT_PULLUP);
  Serial.begin(115200);

  // FALLING — спрацювання при натисканні (перехід з 1 в 0)
  attachInterrupt(digitalPinToInterrupt(BUTTON_IN), button_isr, FALLING);
  Serial.println("Ready. Press the button 10 times.");
}

void loop() {
  // Копіюємо змінні з ISR атомарно
  noInterrupts();
  uint32_t edges = press_edges;
  uint32_t first = first_edge_us;
  uint32_t last = last_edge_us;
  uint32_t total = button_counter;
  interrupts();

  // Натискання завершене: фронти були, але вже QUIET_MS тиша
  if (edges > 0 && (micros() - last) > QUIET_MS * 1000) {
    noInterrupts();
    press_edges = 0;
    interrupts();

    press_number++;
    Serial.printf("Press #%2u | FALLING edges: %3u | bounce: %7.3f ms | total count: %u\n",
                  press_number, edges, (last - first) / 1000.0f, total);
  }

  delay(10);
}

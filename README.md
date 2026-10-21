# Beetroot Academy — Embedded Development (ESP32-S3)

PlatformIO-проєкт для ESP32-S3 DevKitC-1 (Arduino framework).
У `src/` + `include/` — поточне домашнє завдання, попередні — в `homework/`.

## Поточне ДЗ: Blink в стилі Embedded C++

### Схема
| Компонент | Підключення |
|---|---|
| LED | GPIO18 → резистор 220 Ом → LED → GND |
| Кнопка | GPIO17 ↔ кнопка ↔ GND (внутрішня підтяжка `INPUT_PULLUP`) |

### Що реалізовано
1. **Blink в Embedded C++**
   - `enum class LedState { Off, On }` — стан світлодіода;
   - клас `Led` з методами `init()` і `set(LedState)` ([include/Led.h](include/Led.h));
   - без `delay()`: неблокуючий superloop на `millis()`;
   - без глобальних змінних: увесь стан у класі `App` в anonymous namespace ([src/main.cpp](src/main.cpp)).
2. **static constexpr замість магічних чисел** — структура `Config` ([include/Config.h](include/Config.h)): піни, інтервал блимання, антидребезг, швидкість Serial.
3. **Вимірювання часу superloop** — `LoopProfiler` раз на 1000 ітерацій друкує середній, мінімальний і максимальний час однієї ітерації `loop()`.
4. **Кнопка через переривання**
   - `attachInterrupt(..., FALLING)`; ISR лише ставить `static volatile bool` прапорець;
   - антидребезг і перемикання режимів — у `loop()`: натискання приймається, лише якщо кнопка перед цим була стабільно відпущена 50 мс;
   - режими по колу: **Блимання → Постійно увімкнено → Постійно вимкнено → Блимання**.

### Концепції
- `volatile` — прапорець, який змінює ISR;
- `const` / `constexpr` / `static` — конфігурація, піни, константи класів;
- без динамічної пам'яті (`new`, `malloc`) і STL-контейнерів;
- superloop-архітектура: кожна задача — короткий неблокуючий крок у `loop()`.

### Формат виводу в Serial Monitor
```
Ready. Mode: Blink
loop: avg <X> us | min <Y> us | max <Z> us (1000 iterations)
Button -> mode: Always ON
Button -> mode: Always OFF
Button -> mode: Blink
```

## Попередні завдання (`homework/`)
- `headlight_hazard/` — Варіант 2: адаптивна фара (LDR) + аварійна сигналізація, схема та симуляція Wokwi;
- `1.5_bounce.cpp` — дослідження брязкоту кнопки через переривання;
- `1.6_ldr_alt.cpp` — LDR, АЦП і похибка `analogRead()` vs `analogReadMilliVolts()`;
- `police_flasher.cpp` — поліцейська мигалка.

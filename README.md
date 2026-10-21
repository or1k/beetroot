# Beetroot Academy — Embedded Development (ESP32-S3)

PlatformIO-проєкт для ESP32-S3 DevKitC-1 (Arduino framework).
У `src/` + `include/` — поточне домашнє завдання, попередні — в `homework/`.

## Поточне ДЗ: Blink в стилі Embedded C++

### Схема
| Компонент | Підключення |
|---|---|
| LED | GPIO18 → резистор 220 Ом → LED → GND |
| Кнопка | GPIO17 ↔ кнопка ↔ GND (внутрішня підтяжка `INPUT_PULLUP`) |

### Файли
- [include/Config.h](include/Config.h) — структура `Config` зі `static constexpr` константами (піни, інтервали);
- [include/Led.h](include/Led.h), [src/Led.cpp](src/Led.cpp) — `enum class LedState` і клас `Led` з `init()` та `set()`;
- [src/main.cpp](src/main.cpp) — superloop: кнопка, блимання, вимірювання часу `loop()`.

### Що реалізовано
1. **Blink без `delay()`** — блимання через `millis()`, `loop()` ніколи не зупиняється.
2. **Без магічних чисел** — усі числа винесені в `Config`.
3. **Час виконання `loop()`** — раз на 1000 ітерацій у Serial друкується середній час однієї ітерації.
4. **Кнопка через переривання** — ISR лише ставить прапорець `volatile bool buttonPressed`.
   Антидребезг і перемикання режимів — у `loop()`.
   Режими по колу: **Блимання → Постійно увімкнено → Постійно вимкнено → Блимання**.

### Концепції
- `volatile` — змінна, яку змінює переривання;
- `static constexpr` — константи конфігурації;
- `static` у функціях — змінні, що зберігають значення між викликами (замість зайвих глобальних);
- `enum class` — стани LED і режими роботи;
- без динамічної пам'яті та STL-контейнерів.

### Формат виводу в Serial Monitor
```
Ready. Mode: Blink
Average loop time: <X> us
Mode: Always ON
Mode: Always OFF
Mode: Blink
```

## Попередні завдання (`homework/`)
- `headlight_hazard/` — Варіант 2: адаптивна фара (LDR) + аварійна сигналізація, схема та симуляція Wokwi;
- `1.5_bounce.cpp` — дослідження брязкоту кнопки через переривання;
- `1.6_ldr_alt.cpp` — LDR, АЦП і похибка `analogRead()` vs `analogReadMilliVolts()`;
- `police_flasher.cpp` — поліцейська мигалка.

# Beetroot Academy — Embedded Development (ESP32-S3)

PlatformIO-проєкт для ESP32-S3 DevKitC-1.
У `src/` — поточне завдання, попередні — в `homework/`.

## Поточне ДЗ 3.4: Програвання звуку на зумері (PWM) — ESP-IDF

Виконано **Завдання 1** — неблокуючий аудіо-плеєр ([src/main.c](src/main.c)). Як бонус — мелодії із Завдання 2.

| Вимога | Як виконано |
|---|---|
| Працює через PWM (LEDC) | частота ШІМ = частота ноти (`ledc_set_freq`), заповнення 50 % |
| Фіксований часовий тік | `esp_timer` кожні **50 мс**; тривалість ноти задається в тіках |
| Не блокує програму | плеєр — машина станів `IDLE → NOTE → GAP → NOTE …`: на кожному тіку лише зменшує лічильник і перемикає стан |
| Без FreeRTOS task і delay | `app_main` лише налаштовує LEDC і таймер та **завершується**; уся робота — в колбеку таймера |

Що програма не заблокована, видно одразу: поки грає мелодія, від того ж тіку незалежно блимає LED (500 мс),
а команди з Serial Monitor обробляються миттєво (читання UART з таймаутом 0).

Мелодії (ноти C4…B4: 262…494 Гц), команди в Serial Monitor:

| Команда | Мелодія |
|---|---|
| `1` | Baby Shark — C D E \| C D E \| C D E |
| `2` | Jingle Bells — E E E \| E E E \| E G C D E |
| `3` | We Will Rock You — C C D \| C C D \| C C D E |
| `4` | Twinkle Twinkle Little Star — C C G G \| A A G |
| `5` | Old MacDonald — G G G D E E D |
| `6` | Twinkle Twinkle повністю — мелодія з уроку (гілка `PFM`); там вона грається блокуюче через `HAL_Delay`, тут — на тіках |
| `s` | стоп |

### Схема
| Елемент | Підключення |
|---|---|
| Пасивний зумер | (+) → GPIO5, (−) → GND |
| LED | GPIO4 → 220 Ом → LED → GND |

### Результат
Serial Monitor:
```
I (279) main_task: Calling app_main()
Buzzer player. Tick 50 ms. Commands:
  1 - Baby Shark
  2 - Jingle Bells
  3 - We Will Rock You
  4 - Twinkle Twinkle Little Star
  5 - Old MacDonald
  6 - Twinkle Twinkle (full, from lesson)
  s - stop
Playing: Baby Shark
I (292) main_task: Returned from app_main()
Done: Baby Shark
2
Playing: Jingle Bells
Done: Jingle Bells
3
Playing: We Will Rock You
Done: We Will Rock You
4
Playing: Twinkle Twinkle Little Star
Done: Twinkle Twinkle Little Star
```
`Returned from app_main()` з'являється **до** `Done: Baby Shark`: `app_main` уже завершився, а мелодія доігрує лише від таймера —
жодної власної задачі FreeRTOS і жодного `delay`.

Перевірено на платі ESP32-S3 з модулем пасивного зумера:
- після старту одразу грає Baby Shark, команди `1`–`6` у Serial Monitor перемикають мелодії, `s` миттєво зупиняє звук;
- під час програвання LED рівно блимає кожні 500 мс — програма не блокується, хоча мелодія триває кілька секунд;
- мелодія з уроку (`6`), яка в прикладі на STM32 блокувала програму через `HAL_Delay`, тут грається на тіках по 50 мс.
## Попередні завдання (`homework/`)
- `pwm/` — ДЗ 3.3: потенціометр керує яскравістю LED і швидкістю двигуна (LEDC);
- `adc_calibration/` — ДЗ 3.2: калібрування ADC, похибка формули, графік нелінійності;
- `sma/` — ДЗ 3.1: SMA-фільтр для LDR на ESP-IDF, гістерезис;
- `miniproject_automotive/` — міні-проєкт (модуль 2, урок 7): замок, аварійка, поворотники, датчик удару; DL → CL → BL;
- `debounce_idf/` — ДЗ 2.6: debounce на ESP-IDF, state-based на апаратному таймері `gptimer`;
- `timers/` — ДЗ 2.5: апаратний таймер керує витяжним вентилятором (реле), watchdog, лог, LED;
- `debounce/` — ДЗ 2.4: 4 методи debounce + RC-фільтр, порівняльна таблиця;
- `superloop/` — ДЗ 2.3: три незалежні LED (200 / 500 / 1000 мс) у superloop на `millis()`;
- `relay/` — вимірювання часу спрацювання реле (`micros()` + переривання), результати 10 вимірювань;
- `motor_control/` — код з уроку: PWM-керування DC-мотором через транзистор, команди off / low / mid / high з Serial;
- `embedded_blink/` — Blink в стилі Embedded C++: `enum class`, `static constexpr`, кнопка через переривання, час `loop()`;
- `headlight_hazard/` — Варіант 2: адаптивна фара (LDR) + аварійна сигналізація, схема та симуляція Wokwi;
- `1.5_bounce.cpp` — дослідження брязкоту кнопки через переривання;
- `1.6_ldr_alt.cpp` — LDR, АЦП і похибка `analogRead()` vs `analogReadMilliVolts()`;
- `police_flasher.cpp` — поліцейська мигалка.

# ДЗ 3.4: Програвання звуку на зумері (PWM) — ESP-IDF

Збірка ESP-IDF: `framework = espidf` у platformio.ini, `CMakeLists.txt` і `sdkconfig.*` з цієї папки — у корінь проєкту, `main.c` і `src_CMakeLists.txt` (як `CMakeLists.txt`) — у `src/`. Так само збираються ДЗ `sma/`, `adc_calibration/`, `pwm/` (для них у `src/CMakeLists.txt` додати `esp_adc` у REQUIRES).


Виконано **Завдання 1** — неблокуючий аудіо-плеєр ([src/main.c](main.c)). Як бонус — мелодії із Завдання 2.

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

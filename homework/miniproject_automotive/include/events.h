#pragma once

#include <Arduino.h>
#include <esp_timer.h>

// Уся програма працює на подіях: переривання кнопок і таймери кладуть подію в чергу,
// loop() спить, поки подія не прийде, і передає її адресату. Ні polling, ні delay().

enum class EventType : uint8_t
{
    Edge,          // кнопка: фронт у перериванні
    DebounceDone,  // кнопка: 30 мс без фронтів — рівень стабільний
    LongTimeout,   // клік: кнопку тримають 2000 мс
    DoubleTimeout, // клік: другого кліку не було
    BlinkTick,     // поворотники: час перемкнути LED
    BrakeTimeout   // стоп-сигнал: час вимкнути
};

// Будь-який компонент, який отримує події, реалізує цей інтерфейс
class EventHandler
{
public:
    virtual void handleEvent(EventType type) = 0;
};

struct Event
{
    EventType type;
    EventHandler *target;
};

void eventsInit();
void postEvent(EventHandler *target, EventType type);                   // з таймерів (звичайний контекст)
void IRAM_ATTR postEventFromISR(EventHandler *target, EventType type);  // з переривань
void waitAndDispatchEvent();                                            // loop(): чекати подію і виконати

// Таймер esp_timer, який по закінченню кладе подію в чергу
class Timer
{
public:
    void init(EventHandler *target, EventType type, const char *name);
    void startOnce(u32_t ms); // перезапускає, якщо вже йде
    void startPeriodic(u32_t ms);
    void stop();

private:
    static void callback(void *arg);

    esp_timer_handle_t handle = nullptr;
    EventHandler *target = nullptr;
    EventType type = EventType::Edge;
};

#include "events.h"

static QueueHandle_t eventQueue = nullptr;

void eventsInit()
{
    eventQueue = xQueueCreate(32, sizeof(Event));
}

void postEvent(EventHandler *target, EventType type)
{
    Event e = {type, target};
    xQueueSend(eventQueue, &e, 0);
}

void IRAM_ATTR postEventFromISR(EventHandler *target, EventType type)
{
    Event e = {type, target};
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(eventQueue, &e, &woken); // якщо черга повна (сильний брязкіт) — подію просто втрачаємо
    if (woken)
    {
        portYIELD_FROM_ISR();
    }
}

void waitAndDispatchEvent()
{
    Event e;
    // Задача "спить" (не витрачає процесор), поки в черзі нема подій — аналог __WFI() з уроку
    if (xQueueReceive(eventQueue, &e, portMAX_DELAY) == pdTRUE)
    {
        e.target->handleEvent(e.type);
    }
}

// ---------- Timer ----------

void Timer::init(EventHandler *t, EventType ty, const char *name)
{
    target = t;
    type = ty;
    esp_timer_create_args_t args = {};
    args.callback = &Timer::callback;
    args.arg = this;
    args.name = name;
    esp_timer_create(&args, &handle);
}

void Timer::callback(void *arg)
{
    Timer *timer = static_cast<Timer *>(arg);
    postEvent(timer->target, timer->type);
}

void Timer::startOnce(u32_t ms)
{
    esp_timer_stop(handle); // якщо не запущений — повертає помилку, ігноруємо
    esp_timer_start_once(handle, (uint64_t)ms * 1000);
}

void Timer::startPeriodic(u32_t ms)
{
    esp_timer_stop(handle);
    esp_timer_start_periodic(handle, (uint64_t)ms * 1000);
}

void Timer::stop()
{
    esp_timer_stop(handle);
}

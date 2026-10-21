#include <Arduino.h>
#include "Config.h"
#include "Led.h"
#include "Button.h"
#include "LoopProfiler.h"

// «Класичний Blink», переписаний в стилі Embedded C++:
//  - enum class для станів, static constexpr замість магічних чисел;
//  - жодного delay(): неблокуючий superloop на millis();
//  - без динамічної пам'яті та STL-контейнерів;
//  - кнопка через переривання перемикає режим: Блимання -> Увімкнено -> Вимкнено -> Блимання.

namespace
{

enum class LedMode : uint8_t
{
    Blink,
    AlwaysOn,
    AlwaysOff
};

const char *toString(LedMode mode)
{
    switch (mode)
    {
    case LedMode::Blink:     return "Blink";
    case LedMode::AlwaysOn:  return "Always ON";
    case LedMode::AlwaysOff: return "Always OFF";
    }
    return "?";
}

LedMode next(LedMode mode)
{
    switch (mode)
    {
    case LedMode::Blink:     return LedMode::AlwaysOn;
    case LedMode::AlwaysOn:  return LedMode::AlwaysOff;
    case LedMode::AlwaysOff: return LedMode::Blink;
    }
    return LedMode::Blink;
}

// Увесь стан програми зібрано в одному об'єкті замість набору глобальних змінних
class App
{
public:
    void setup()
    {
        Serial.begin(Config::kSerialBaud);
        led_.init();
        button_.init();
        Serial.printf("Ready. Mode: %s\n", toString(mode_));
    }

    void loop()
    {
        profiler_.tick(micros());
        const uint32_t now = millis();

        if (button_.poll(now))
        {
            mode_ = next(mode_);
            lastToggleMs_ = now;
            Serial.printf("Button -> mode: %s\n", toString(mode_));

            // Постійні режими достатньо застосувати один раз — при перемиканні
            if (mode_ == LedMode::AlwaysOn)
            {
                led_.set(LedState::On);
            }
            else if (mode_ == LedMode::AlwaysOff)
            {
                led_.set(LedState::Off);
            }
        }

        if (mode_ == LedMode::Blink && now - lastToggleMs_ >= Config::kBlinkIntervalMs)
        {
            lastToggleMs_ = now;
            led_.toggle();
        }
    }

private:
    Led led_{Config::kLedPin};
    Button button_{Config::kButtonPin};
    LoopProfiler profiler_{Config::kProfileEveryIterations};

    LedMode mode_ = LedMode::Blink;
    uint32_t lastToggleMs_ = 0;
};

App app; // єдиний об'єкт на рівні файлу; anonymous namespace — не видно з інших файлів

} // namespace

void setup()
{
    app.setup();
}

void loop()
{
    app.loop();
}

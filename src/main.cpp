#include <Arduino.h>
#include "config.h"
#include "events.h"
#include "dl/Button.h"
#include "cl/ClickDetector.h"
#include "cl/Blinker.h"
#include "cl/Lock.h"
#include "cl/BrakeLight.h"
#include "bl/Car.h"

// Міні-проєкт Automotive: замок, аварійка, поворотники, датчик удару.
// Шари: DL (Button, Output) -> CL (ClickDetector, Blinker, Lock, BrakeLight) -> BL (Car).

// ---------- DL ----------
Button lockButton, hazardButton, turnButton, crashSensor;

// ---------- CL ----------
ClickDetector lockClicks, hazardClicks, turnClicks, crashClicks;
Blinker blinker;
Lock lock;
BrakeLight brake;

// ---------- BL ----------
Car car;

void setup()
{
    Serial.begin(115200);
    eventsInit();

    // Знизу нагору: спочатку виходи, потім бізнес-логіка, потім кнопки (щоб події не прийшли раніше)
    lock.init(LOCK_RELAY_PIN);
    blinker.init(LEFT_LED_PIN, RIGHT_LED_PIN);
    brake.init(BRAKE_LED_PIN);
    car.init(&lock, &blinker, &brake);

    lockClicks.init(ButtonId::Lock, false, &car);
    hazardClicks.init(ButtonId::Hazard, false, &car);
    turnClicks.init(ButtonId::Turn, true, &car); // лише поворотнику потрібен подвійний клік
    crashClicks.init(ButtonId::Crash, false, &car);

    lockButton.init(LOCK_BUTTON_PIN, &lockClicks);
    hazardButton.init(HAZARD_BUTTON_PIN, &hazardClicks);
    turnButton.init(TURN_BUTTON_PIN, &turnClicks);
    crashSensor.init(CRASH_SENSOR_PIN, &crashClicks);

    Serial.println("Automotive mini project ready");
}

void loop()
{
    // Без polling і без delay(): чекаємо наступну подію (переривання кнопки або таймер)
    waitAndDispatchEvent();
}

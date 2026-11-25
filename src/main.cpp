#include <Arduino.h>
#include "config.h"
#include "events.h"
#include "dl/Button.h"
#include "dl/Encoder.h"
#include "cl/ClickDetector.h"
#include "cl/Blinker.h"
#include "cl/Lock.h"
#include "cl/BrakeLight.h"
#include "cl/LightSensor.h"
#include "cl/Throttle.h"
#include "cl/Headlight.h"
#include "cl/Motor.h"
#include "cl/Mirror.h"
#include "bl/Car.h"

// Міні-проєкт Automotive (модулі 2 і 3).
// Модуль 2: замок, аварійка, поворотники, датчик удару.
// Модуль 3: автоматичні фари (LDR), дзеркало на серво з енкодером, швидкість двигуна від потенціометра.
// Шари: DL (Button, Encoder, Output, PwmOutput, Servo)
//    -> CL (ClickDetector, Blinker, Lock, BrakeLight, LightSensor, Throttle, Headlight, Motor, Mirror, Sma)
//    -> BL (Car).

// ---------- DL ----------
Button lockButton, hazardButton, turnButton, crashSensor, mirrorButton;
Encoder encoder;

// ---------- CL ----------
ClickDetector lockClicks, hazardClicks, turnClicks, crashClicks, mirrorClicks;
Blinker blinker;
Lock lock;
BrakeLight brake;
Headlight headlight;
Motor motor;
Mirror mirror;
LightSensor lightSensor;
Throttle throttle;

// ---------- BL ----------
Car car;

void setup()
{
    Serial.begin(115200);
    eventsInit();

    // Знизу нагору: спочатку виходи, потім бізнес-логіка, потім входи (щоб події не прийшли раніше)
    lock.init(LOCK_RELAY_PIN);
    blinker.init(LEFT_LED_PIN, RIGHT_LED_PIN);
    brake.init(BRAKE_LED_PIN);
    headlight.init(HEADLIGHT_PIN);
    motor.init(MOTOR_PIN);
    mirror.init(SERVO_PIN);

    car.init({&lock, &blinker, &brake, &headlight, &motor, &mirror});

    lockClicks.init(ButtonId::Lock, false, &car);
    hazardClicks.init(ButtonId::Hazard, false, &car);
    turnClicks.init(ButtonId::Turn, true, &car); // лише поворотнику потрібен подвійний клік
    crashClicks.init(ButtonId::Crash, false, &car);
    mirrorClicks.init(ButtonId::Mirror, false, &car);

    lockButton.init(LOCK_BUTTON_PIN, &lockClicks);
    hazardButton.init(HAZARD_BUTTON_PIN, &hazardClicks);
    turnButton.init(TURN_BUTTON_PIN, &turnClicks);
    crashSensor.init(CRASH_SENSOR_PIN, &crashClicks);
    mirrorButton.init(ENCODER_SW_PIN, &mirrorClicks);
    encoder.init(ENCODER_CLK_PIN, ENCODER_DT_PIN, &car);

    lightSensor.init(LDR_PIN, &car);
    throttle.init(POT_PIN, &car);

    Serial.println("Automotive mini project (modules 2 + 3) ready");
}

void loop()
{
    // Без polling і без delay(): чекаємо наступну подію (переривання, таймер кнопки або таймер датчика)
    waitAndDispatchEvent();
}

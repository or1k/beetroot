#include <Arduino.h>

// Завдання: LDR (фоторезистор) у подільнику напруги, вимірювання АЦП.
//
// Схема подільника:
//   3V3 ── LDR ──┬── резистор 10 кОм ── GND
//                │
//              GPIO4 (ADC1)
// Більше світла -> менший опір LDR -> вища напруга на GPIO4.

#define LDR_PIN 4   // GPIO4 = ADC1_CH3 (ADC1 працює і разом з Wi-Fi)

const u32_t ADC_MAX = 4095;    // 12-бітний АЦП: 0..4095
const u32_t U_REF_MV = 3100;   // опорна напруга, мВ (3.1 В)
const u32_t PERIOD_MS = 100;   // зчитування кожні 100 мс

u32_t lastRead = 0;
u32_t lineCount = 0;

void printHeader() {
    Serial.println();
    Serial.println("|  RAW  | U calc, мВ | U meas, мВ | Похибка, % |");
    Serial.println("|-------|------------|------------|------------|");
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);                    // 12 біт -> 0..4095
    analogSetPinAttenuation(LDR_PIN, ADC_11db);  // діапазон ~0..3.1 В
    delay(1000);                                 // час відкрити Serial Monitor
}

void loop() {
    if (millis() - lastRead < PERIOD_MS) return;
    lastRead = millis();

    // 3. Сире значення АЦП
    u32_t raw = analogRead(LDR_PIN);

    // 4. Обчислена напруга: U = RAW / ADCmax * Uref
    float uCalc = (float)raw / ADC_MAX * U_REF_MV;

    // 5. Напруга з урахуванням заводської калібровки АЦП
    u32_t uMeas = analogReadMilliVolts(LDR_PIN);

    // 6. Відносна похибка обчислень у відсотках (відносно виміряної напруги)
    float error = (uMeas > 0) ? fabs(uCalc - uMeas) / uMeas * 100.0f : 0.0f;

    // 7. Вивід у вигляді таблиці (заголовок кожні 20 рядків)
    if (lineCount % 20 == 0) printHeader();
    lineCount++;

    Serial.printf("| %5u | %10.1f | %10u | %10.2f |\n", raw, uCalc, uMeas, error);
}

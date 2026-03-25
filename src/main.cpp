#include <Arduino.h>

#define MQ135_PIN A0
#define RL 10.0

#define NUM_SAMPLES 10
#define CALIBRATION_SAMPLES 20

float R0 = 76.63; // valor inicial (vai ser recalculado)
/* 🔢 Valores típicos (com RL = 10kΩ) em ar limpo (~400 ppm CO₂):
            - 50 kΩ → 150 kΩ → faixa mais comum
            - 60 kΩ → 90 kΩ → faixa bem “normal” (ideal)
            - ~76 kΩ → valor clássico de referência (igual o seu)*/
            
// =====================
// MÉDIA DE LEITURAS
// =====================
float readAverage() {
    long sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(MQ135_PIN);
        delay(50);
    }

    return sum / (float)NUM_SAMPLES;
}

// =====================
// CALCULA RS
// =====================
float calculateRS(float voltage) {
    return ((5.0 / voltage) - 1.0) * RL;
}

// =====================
// CALCULA CO2
// =====================
float calculateCO2(float ratio) {
    return 116.6020682 * pow(ratio, -2.769034857);
}

// =====================
// CALIBRAÇÃO DO R0
// =====================
float calibrateR0() {
    Serial.println("Calibrando R0... (mantenha em ar limpo)");

    float sum = 0;

    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        float analogValue = readAverage();
        float voltage = analogValue * (5.0 / 1023.0);
        float RS = calculateRS(voltage);

        float r0 = RS / 3.6; // fator de ar limpo

        sum += r0;

        Serial.print("Amostra ");
        Serial.print(i + 1);
        Serial.print(": R0 = ");
        Serial.println(r0);

        delay(1000);
    }

    float r0_final = sum / CALIBRATION_SAMPLES;

    Serial.println("----------------------");
    Serial.print("R0 calibrado: ");
    Serial.println(r0_final);
    Serial.println("----------------------");

    return r0_final;
}

// =====================
// SETUP
// =====================
void setup() {
    Serial.begin(9600);
    pinMode(MQ135_PIN, INPUT);

    delay(5000); // tempo inicial (sensor estabilizar)

    R0 = calibrateR0(); // 🔥 calibração automática
}

// =====================
// LOOP PRINCIPAL
// =====================
void loop() {

    float analogValue = readAverage();
    float voltage = analogValue * (5.0 / 1023.0);

    float RS = calculateRS(voltage);
    float ratio = RS / R0;

    float co2ppm = calculateCO2(ratio);

    Serial.print("Tensao: ");
    Serial.print(voltage);
    Serial.print(" V | RS: ");
    Serial.print(RS);
    Serial.print(" kOhm | CO2: ");
    Serial.print(co2ppm);
    Serial.println(" PPM");

    delay(2000);
}
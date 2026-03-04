#include<Arduino.h>

#define MQ135_PIN A0
#define RL 10.0          // Resistor de carga em kΩ
#define R0 76.63         // Valor calibrado em ar limpo (ajuste depois)

#define NUM_SAMPLES 10

float readAverage() {
    long sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(MQ135_PIN);
        delay(50);
    }

    return sum / (float)NUM_SAMPLES;
}

float calculateRS(float voltage) {
    return ((5.0 / voltage) - 1.0) * RL;
}

float calculateCO2(float ratio) {
    return 116.6020682 * pow(ratio, -2.769034857);
}

void setup() {
    Serial.begin(9600);
    pinMode(MQ135_PIN, INPUT);
}

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
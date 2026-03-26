#include <Arduino.h>

// ============================================================
// CONFIGURAÇÕES — AJUSTE AQUI
// ============================================================
#define MQ135_PIN    A0
#define RL_OHMS      10000.0   // Resistor de carga do módulo (10kΩ típico)
#define VCC          5.0       // Tensão de alimentação
#define RATIO_CLEAN  3.6       // RS/R0 em ar limpo (datasheet MQ-135)
#define NUM_AMOSTRAS 200       // Amostras para calibração (mais = mais preciso)
#define INTERVALO_MS 50        // Intervalo entre amostras

// ============================================================
// VARIÁVEIS GLOBAIS
// ============================================================
float R0 = 10.0;  // Será atualizado após calibração (kΩ)

// ============================================================
// FUNÇÕES UTILITÁRIAS
// ============================================================

float lerTensao() {
    return analogRead(MQ135_PIN) * (VCC / 1023.0);
}

float calcularRS(float tensao) {
    if (tensao <= 0.001) return 999999.0;  // evita divisão por zero
    return RL_OHMS * (VCC - tensao) / tensao;
}

// ============================================================
// CALIBRAÇÃO — CHAMAR UMA VEZ EM AR LIMPO
// ============================================================
float calibrarR0() {
    Serial.println();
    Serial.println("============================================");
    Serial.println("   CALIBRACAO MQ-135");
    Serial.println("   Deixe o sensor em ar limpo/ventilado");
    Serial.println("============================================");
    Serial.println("Coletando amostras...");

    long somaADC = 0;
    float minRS = 999999, maxRS = 0;

    for (int i = 0; i < NUM_AMOSTRAS; i++) {
        float v = lerTensao();
        float rs = calcularRS(v);

        somaADC += analogRead(MQ135_PIN);

        if (rs < minRS) minRS = rs;
        if (rs > maxRS) maxRS = rs;

        if (i % 20 == 0) {
            Serial.print("  ["); Serial.print(i); Serial.print("/");
            Serial.print(NUM_AMOSTRAS); Serial.print("] RS = ");
            Serial.print(rs / 1000.0, 2); Serial.println(" kOhm");
        }

        delay(INTERVALO_MS);
    }

    // Faz leitura final mais limpa (descarta primeiros transitórios)
    long somaRS = 0;
    for (int i = 0; i < 50; i++) {
        float v = lerTensao();
        somaRS += (long)calcularRS(v);
        delay(INTERVALO_MS);
    }

    float rsMedia = somaRS / 50.0;
    float r0Calculado = rsMedia / RATIO_CLEAN;

    Serial.println();
    Serial.println("---- RESULTADO DA CALIBRACAO ----");
    Serial.print("RS media (ar limpo): ");
    Serial.print(rsMedia / 1000.0, 2); Serial.println(" kOhm");

    Serial.print("R0 calculado:        ");
    Serial.print(r0Calculado / 1000.0, 2); Serial.println(" kOhm");

    Serial.print("Dispersao RS (max-min): ");
    Serial.print((maxRS - minRS) / 1000.0, 2); Serial.println(" kOhm");

    // Diagnóstico de confiabilidade
    float dispersaoPct = ((maxRS - minRS) / rsMedia) * 100.0;
    Serial.print("Dispersao relativa: ");
    Serial.print(dispersaoPct, 1); Serial.println("%");

    if (dispersaoPct < 5.0) {
        Serial.println(">> Ambiente estavel, calibracao CONFIAVEL");
    } else if (dispersaoPct < 15.0) {
        Serial.println(">> Variacao moderada, calibracao ACEITAVEL");
    } else {
        Serial.println(">> ATENCAO: alta variacao. Repita em local mais estavel.");
    }

    Serial.println();
    Serial.print(">>> GRAVE NO CODIGO: #define R0_FIXO ");
    Serial.println(r0Calculado / 1000.0, 2);
    Serial.println("=================================");

    return r0Calculado;
}

// ============================================================
// LEITURA DE QUALIDADE DO AR
// ============================================================
void lerQualidadeAr() {
    // Coleta média de 10 leituras para estabilidade
    long soma = 0;
    for (int i = 0; i < 10; i++) {
        soma += analogRead(MQ135_PIN);
        delay(20);
    }
    float adc = soma / 10.0;
    float tensao = adc * (VCC / 1023.0);
    float rs = calcularRS(tensao);
    float ratio = rs / R0;  // RS/R0

    // Índice de qualidade relativa (quanto MAIOR, mais limpo)
    // Valores típicos MQ-135:
    //   > 3.6  → ar muito limpo
    //   2.0–3.6 → ar normal (CO2 ambiente)
    //   1.0–2.0 → contaminação moderada
    //   < 1.0  → contaminação alta

    String qualidade;
    if (ratio > 3.6)      qualidade = "Muito limpo";
    else if (ratio > 2.5) qualidade = "Limpo";
    else if (ratio > 1.5) qualidade = "Moderado";
    else if (ratio > 1.0) qualidade = "Ruim";
    else                  qualidade = "MUITO RUIM";

    Serial.println("--------------------------");
    Serial.print("ADC: ");     Serial.println((int)adc);
    Serial.print("Tensao: ");  Serial.print(tensao, 3); Serial.println(" V");
    Serial.print("RS: ");      Serial.print(rs / 1000.0, 2); Serial.println(" kOhm");
    Serial.print("RS/R0: ");   Serial.println(ratio, 3);
    Serial.print("Qualidade: "); Serial.println(qualidade);
    Serial.println("--------------------------");
}

// ============================================================
// SETUP E LOOP
// ============================================================
void setup() {
    Serial.begin(9600);
    pinMode(MQ135_PIN, INPUT);

    Serial.println();
    Serial.println("MQ-135 — Sistema de calibracao");
    Serial.println("Aguardando sensor aquecer (30s)...");
    delay(30000);  // Mínimo para testes; ideal é 24h para primeira vez

    // Faz a calibração automaticamente no primeiro boot
    R0 = calibrarR0();

    Serial.println();
    Serial.println("Iniciando monitoramento de qualidade do ar...");
    Serial.println();
}

void loop() {
    lerQualidadeAr();
    delay(3000);
}
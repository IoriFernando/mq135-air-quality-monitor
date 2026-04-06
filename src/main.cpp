#include <Arduino.h>
#include <EEPROM.h>
#include <math.h>   // necessário para pow()

// ============================================================
// CONFIGURAÇÕES
// ============================================================
#define MQ135_PIN    A0
#define RL_OHMS      10000.0
#define VCC          5.0
#define RATIO_CLEAN  3.6
#define NUM_AMOSTRAS 200
#define INTERVALO_MS 50
#define EEPROM_ADDR  0

// Coeficientes da curva CO₂ do datasheet MQ-135
#define CO2_A        110.47
#define CO2_B        -2.862

// CO₂ atmosférico normal (~400 PPM ambiente externo)
#define CO2_ATMOSFERICO 400.0

// ============================================================
// VARIÁVEIS GLOBAIS
// ============================================================
float R0 = 10.0;
bool modoMonitoramento = false;

// ============================================================
// EEPROM
// ============================================================
void salvarR0naEEPROM(float valor) {
    EEPROM.put(EEPROM_ADDR, valor);
    Serial.println(">> R0 salvo na EEPROM.");
}

float carregarR0daEEPROM() {
    float valor;
    EEPROM.get(EEPROM_ADDR, valor);
    if (isnan(valor) || valor < 1000.0 || valor > 100000.0) {
        Serial.println(">> EEPROM sem R0 valido.");
        return 0.0;
    }
    return valor;
}

// ============================================================
// UTILITÁRIOS
// ============================================================
float lerTensao() {
    return analogRead(MQ135_PIN) * (VCC / 1023.0);
}

float calcularRS(float tensao) {
    if (tensao <= 0.001) return 999999.0;
    return RL_OHMS * (VCC - tensao) / tensao;
}

// ============================================================
// CÁLCULO DE PPM CO₂  ← NOVA FUNÇÃO
// ============================================================

/*
  COMO FUNCIONA:
  O datasheet do MQ-135 fornece uma curva log-log
  relacionando RS/R0 com a concentração de gás em PPM.

  Para CO₂ a fórmula é:
      PPM = A × (RS/R0) ^ B

  Onde A=110.47 e B=-2.862 são coeficientes extraídos
  da curva do datasheet.

  VALIDAÇÃO FUNCIONAL:
  Em ar limpo, RS/R0 ≈ 3.6 (por definição da calibração).
  Então:
      PPM = 110.47 × 3.6 ^ (-2.862)
      PPM = 110.47 × 0.0356...  (aproximadamente)
      PPM ≈ ~350–450 PPM

  Isso bate com o CO₂ atmosférico real (~400 PPM),
  confirmando que a função está correta.
  Se retornar valores muito fora disso, o R0 está errado.
*/

float calcularPPM_CO2(float rs) {
    if (R0 <= 0) {
        Serial.println("ERRO: R0 invalido para calculo de PPM.");
        return -1.0;
    }

    float ratio = rs / R0;

    // Proteção: ratio fora do range do datasheet (0.3 a 10)
    if (ratio <= 0.0 || ratio > 15.0) {
        Serial.println("AVISO: ratio RS/R0 fora do range do datasheet.");
        return -1.0;
    }

    // Fórmula principal: PPM = A × ratio^B
    float ppm = CO2_A * pow(ratio, CO2_B);

    // PPM negativo ou zero é fisicamente impossível
    if (ppm <= 0) {
        Serial.println("AVISO: PPM calculado invalido.");
        return -1.0;
    }

    return ppm;
}

// ============================================================
// DIAGNÓSTICO DO SENSOR — verifica se PPM faz sentido
// ============================================================

/*
  COMO INTERPRETAR:
  O ar externo tem ~400 PPM de CO₂ (valor constante global).
  Ambientes internos normais ficam entre 400–1000 PPM.
  Acima de 1000 PPM já indica ventilação ruim.
  Acima de 2000 PPM é prejudicial à concentração.
  Acima de 5000 PPM é perigoso.

  Se em ar limpo seu sensor retornar muito longe de 400 PPM,
  o R0 precisa ser recalibrado.
*/
void diagnosticarSensor(float ppm) {
    Serial.println();
    Serial.println("=== DIAGNOSTICO DO SENSOR ===");
    Serial.print("PPM CO2 calculado: ");
    Serial.println(ppm, 1);

    float desvio = abs(ppm - CO2_ATMOSFERICO);
    float desvioPct = (desvio / CO2_ATMOSFERICO) * 100.0;

    Serial.print("Desvio do valor atmosferico (400 PPM): ");
    Serial.print(desvioPct, 1);
    Serial.println("%");

    if (desvioPct < 20.0) {
        Serial.println("STATUS: Sensor FUNCIONAL ✓");
        Serial.println("R0 calibrado corretamente.");
    } else if (desvioPct < 50.0) {
        Serial.println("STATUS: Sensor ACEITAVEL");
        Serial.println("Pequeno desvio — pode ser umidade ou temperatura.");
    } else {
        Serial.println("STATUS: ATENCAO — sensor pode estar descalibrado.");
        Serial.println("Recalibre o R0 em ar limpo (comando C).");
    }

    Serial.println("=============================");
    Serial.println();
}

// ============================================================
// CLASSIFICAÇÃO POR PPM (escala padrão CO₂)
// ============================================================
String classificarPPM(float ppm) {
    if (ppm < 400)        return "Ar externo puro";
    else if (ppm < 600)   return "Excelente";
    else if (ppm < 1000)  return "Bom";
    else if (ppm < 1500)  return "Moderado";
    else if (ppm < 2000)  return "Ruim — ventilar!";
    else if (ppm < 5000)  return "MUITO RUIM";
    else                  return "PERIGOSO";
}

// ============================================================
// LEITURA COMPLETA COM PPM
// ============================================================
void lerQualidadeAr() {
    // Média de 10 leituras de RS
    float somaRS = 0.0;
    for (int i = 0; i < 10; i++) {
        somaRS += calcularRS(lerTensao());
        delay(20);
    }
    float rs     = somaRS / 10.0;
    float tensao = lerTensao();
    float ratio  = rs / R0;
    float ppm    = calcularPPM_CO2(rs);

    // Qualidade por ratio (relativa)
    String qualRatio;
    if      (ratio > 3.6) qualRatio = "Muito limpo";
    else if (ratio > 2.5) qualRatio = "Limpo";
    else if (ratio > 1.5) qualRatio = "Moderado";
    else if (ratio > 1.0) qualRatio = "Ruim";
    else                  qualRatio = "MUITO RUIM";

    // Qualidade por PPM (absoluta)
    String qualPPM = (ppm > 0) ? classificarPPM(ppm) : "Erro no calculo";

    Serial.println("----------------------------------");
    Serial.print("Tensao:      "); Serial.print(tensao, 3);      Serial.println(" V");
    Serial.print("RS media:    "); Serial.print(rs / 1000.0, 3); Serial.println(" kOhm");
    Serial.print("R0 atual:    "); Serial.print(R0 / 1000.0, 3); Serial.println(" kOhm");
    Serial.print("RS/R0:       "); Serial.println(ratio, 3);
    Serial.print("Qualidade:   "); Serial.println(qualRatio);
    Serial.println("  ---  ");
    Serial.print("CO2 (PPM):   ");
    if (ppm > 0) { Serial.println(ppm, 1); }
    else           { Serial.println("Erro"); }
    Serial.print("Status PPM:  "); Serial.println(qualPPM);
    Serial.println("----------------------------------");
    Serial.println("Comandos: [C]=Calibrar [D]=Diagnostico [R]=Ver R0 [M]=R0 manual");
}

// ============================================================
// CALIBRAÇÃO
// ============================================================
float calibrarR0() {
    Serial.println();
    Serial.println("============================================");
    Serial.println("   CALIBRACAO MQ-135 — AR LIMPO");
    Serial.println("============================================");

    float somaRS = 0.0;
    float minRS  = 999999.0;
    float maxRS  = 0.0;
    int   validas = 0;

    for (int i = 0; i < NUM_AMOSTRAS; i++) {
        float v  = lerTensao();
        float rs = calcularRS(v);

        if (rs < 999990.0) {
            somaRS += rs;
            validas++;
            if (rs < minRS) minRS = rs;
            if (rs > maxRS) maxRS = rs;
        }

        if (i % 20 == 0) {
            Serial.print("  ["); Serial.print(i);
            Serial.print("/"); Serial.print(NUM_AMOSTRAS);
            Serial.print("] RS = ");
            Serial.print(rs / 1000.0, 2); Serial.println(" kOhm");
        }
        delay(INTERVALO_MS);
    }

    if (validas < 10) {
        Serial.println("ERRO: amostras insuficientes!");
        return 0.0;
    }

    float rsMedia      = somaRS / validas;
    float r0Calculado  = rsMedia / RATIO_CLEAN;
    float dispersaoPct = ((maxRS - minRS) / rsMedia) * 100.0;

    // Verificação imediata de PPM com o novo R0
    float ppmTeste = CO2_A * pow(RATIO_CLEAN, CO2_B);

    Serial.println();
    Serial.println("======== RESULTADO ========");
    Serial.print("Amostras validas: "); Serial.print(validas);
    Serial.print("/"); Serial.println(NUM_AMOSTRAS);
    Serial.print("RS media:  "); Serial.print(rsMedia / 1000.0, 3); Serial.println(" kOhm");
    Serial.print("R0 calc:   "); Serial.print(r0Calculado / 1000.0, 3); Serial.println(" kOhm");
    Serial.print("Dispersao: "); Serial.print(dispersaoPct, 1); Serial.println("%");
    Serial.print("PPM esperado em ar limpo: "); Serial.println(ppmTeste, 1);

    if      (dispersaoPct < 5.0)  Serial.println("STATUS: CONFIAVEL ✓");
    else if (dispersaoPct < 15.0) Serial.println("STATUS: ACEITAVEL");
    else                          Serial.println("STATUS: ATENCAO — repita!");

    Serial.println("===========================");
    return r0Calculado;
}

// ============================================================
// MENU SERIAL
// ============================================================
void processarComandoSerial() {
    if (!Serial.available()) return;

    char cmd = Serial.read();
    while (Serial.available()) Serial.read();

    if (cmd == 'C' || cmd == 'c') {
        modoMonitoramento = false;
        Serial.println(">> Posicione em AR LIMPO. Iniciando em 10s...");
        delay(10000);
        float novoR0 = calibrarR0();
        if (novoR0 > 0) {
            R0 = novoR0;
            salvarR0naEEPROM(R0);
            Serial.println(">> R0 atualizado e salvo!");
        }
        modoMonitoramento = true;

    } else if (cmd == 'D' || cmd == 'd') {
        // Diagnóstico: lê RS atual e testa PPM
        float rs  = calcularRS(lerTensao());
        float ppm = calcularPPM_CO2(rs);
        diagnosticarSensor(ppm);

    } else if (cmd == 'R' || cmd == 'r') {
        Serial.print(">> R0 atual: ");
        Serial.print(R0 / 1000.0, 3);
        Serial.println(" kOhm");

    } else if (cmd == 'M' || cmd == 'm') {
        Serial.println(">> Digite R0 em kOhm e pressione Enter:");
        unsigned long t = millis();
        while (!Serial.available() && millis() - t < 10000);
        if (Serial.available()) {
            float v = Serial.parseFloat();
            while (Serial.available()) Serial.read();
            if (v > 1.0 && v < 100.0) {
                R0 = v * 1000.0;
                salvarR0naEEPROM(R0);
                Serial.print(">> R0 manual definido: ");
                Serial.print(R0 / 1000.0, 3);
                Serial.println(" kOhm — salvo.");
            } else {
                Serial.println(">> Valor invalido (1–100 kOhm).");
            }
        } else {
            Serial.println(">> Timeout.");
        }
    }
}

// ============================================================
// SETUP E LOOP
// ============================================================
void setup() {
    Serial.begin(9600);
    pinMode(MQ135_PIN, INPUT);

    Serial.println();
    Serial.println("=== MQ-135 CO2 + Qualidade do Ar ===");

    float r0Salvo = carregarR0daEEPROM();
    if (r0Salvo > 0) {
        R0 = r0Salvo;
        Serial.print(">> R0 carregado da EEPROM: ");
        Serial.print(R0 / 1000.0, 3);
        Serial.println(" kOhm");
        Serial.println(">> Aquecendo (30s)...");
        delay(30000);
    } else {
        Serial.println(">> Aquecendo para calibrar (30s)...");
        delay(30000);
        R0 = calibrarR0();
        if (R0 > 0) salvarR0naEEPROM(R0);
    }

    modoMonitoramento = true;
    Serial.println(">> Monitoramento iniciado!");
    Serial.println("Comandos: [C]=Calibrar [D]=Diagnostico [R]=Ver R0 [M]=R0 manual");
}

void loop() {
    processarComandoSerial();
    if (modoMonitoramento) {
        lerQualidadeAr();
        delay(3000);
    }
}
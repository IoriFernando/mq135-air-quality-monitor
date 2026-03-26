# 🌫️ MQ-135 - Calibração e Monitoramento da Qualidade do Ar

## 📌 Visão Geral

Este projeto utiliza o sensor MQ-135 para monitorar a qualidade do ar com base em variações relativas de gases.
Devido às limitações do sensor, o sistema não mede CO₂ absoluto com precisão, mas fornece um **indicador confiável da qualidade do ar**.

---

# ⚙️ 🔧 Processo de Calibração

## 🎯 Objetivo

Determinar o valor de **R0**, que representa a resistência do sensor em condições de ar limpo.

---

## 🔄 Etapas da Calibração

1. **Aquecimento do sensor**

   * Tempo mínimo: 30 segundos (teste)
   * Ideal: 24 horas (primeiro uso)

2. **Coleta de amostras**

   * 200 leituras do sensor
   * Intervalo entre leituras: 50 ms

3. **Cálculo da resistência (RS)**

   * Baseado na tensão lida no pino analógico

4. **Filtragem de ruído**

   * Coleta adicional de 50 leituras
   * Média final mais estável

5. **Cálculo do R0**

   * Fórmula:

     ```
     R0 = RS / 3.6
     ```
   * 3.6 representa o fator de ar limpo (datasheet)

6. **Validação da calibração**

   * Análise de dispersão:

     * < 5% → Confiável
     * 5%–15% → Aceitável
     * > 15% → Repetir calibração

---

## 📊 Saída da calibração

Exemplo:

```
RS média: 40 kOhm
R0 calculado: 11.11 kOhm
Dispersão: 3.2%
```

---

# 🧠 🔍 Funcionamento do Código

## 🔹 1. Leitura do Sensor

* O sensor retorna um valor analógico (0–1023)
* Convertido em tensão:

  ```
  V = leitura * (5.0 / 1023.0)
  ```

---

## 🔹 2. Cálculo da resistência (RS)

```
RS = RL * (VCC - V) / V
```

Onde:

* RL = 10kΩ
* VCC = 5V

---

## 🔹 3. Cálculo da razão (RS/R0)

```
ratio = RS / R0
```

👉 Esse é o valor mais importante do sistema

---

## 🔹 4. Classificação da qualidade do ar

| RS/R0     | Qualidade   |
| --------- | ----------- |
| > 3.6     | Muito limpo |
| 2.5 – 3.6 | Limpo       |
| 1.5 – 2.5 | Moderado    |
| 1.0 – 1.5 | Ruim        |
| < 1.0     | Muito ruim  |

---

# ⚠️ Limitações do Sensor

* Não mede CO₂ com precisão absoluta
* Sensível a múltiplos gases:

  * álcool
  * amônia
  * fumaça
* Influenciado por:

  * umidade
  * temperatura

---

# ✅ Vantagens da Abordagem Utilizada

* Mais robusta que cálculo direto em PPM
* Independente de calibração perfeita
* Funciona em ambientes reais (inclusive alta umidade)
* Ideal para monitoramento contínuo

---

# 🚀 Conclusão

Este sistema utiliza o MQ-135 de forma eficiente ao:

✔ Calibrar corretamente o sensor
✔ Trabalhar com dados relativos (RS/R0)
✔ Classificar a qualidade do ar de forma prática

---

# 📌 Observação Final

Para aplicações mais precisas de CO₂, recomenda-se sensores específicos (NDIR).
Este projeto é ideal para **detecção de variação e qualidade do ar ambiente**.

---

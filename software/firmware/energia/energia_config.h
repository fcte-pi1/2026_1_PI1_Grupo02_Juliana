#ifndef ENERGIA_CONFIG_H
#define ENERGIA_CONFIG_H

// ============================================================================
// Configuracao central do monitoramento de energia (HU21 / HU16 / HU10)
// ----------------------------------------------------------------------------
// Todos os parametros calibraveis ficam AQUI. Pra ajustar hardware, bateria ou
// limiares de alerta, mexa so neste arquivo - o resto do codigo consome destas
// constantes. Valores marcados "(a confirmar)" dependem de medicao real da
// eletronica.
// ============================================================================

// --- Sensor de tensao (voltage_sensor) --------------------------------------
// GP26 = ADC0. Divisor resistivo do modulo: tensao_real = tensao_pino * ratio.
#define ENERGIA_TENSAO_GPIO          26
#define ENERGIA_TENSAO_DIVIDER_RATIO 5.0f   // (a confirmar com eletronica)

// --- Sensor de corrente (current_sensor, HW-872 efeito Hall) ----------------
// GP27 = ADC1. Sensibilidade depende do jumper de range do modulo.
// zero_offset e so o chute inicial: calibrado em runtime no boot.
#define ENERGIA_CORRENTE_GPIO        27
#define ENERGIA_CORRENTE_SENS_MV_A   185.0f // jumper em 5A -> 185 mV/A
#define ENERGIA_CORRENTE_ZERO_MV     1650   // VCC/2 p/ alimentacao 3.3V (a confirmar: 3.3V vs 5V)

// --- Bateria (LiPo 2S nominal 7.4V) -----------------------------------------
// Ajuste conforme a bateria real usada no robo.
#define ENERGIA_BAT_TENSAO_CHEIA_V   8.4f   // 100% (4.2V/celula)
#define ENERGIA_BAT_TENSAO_VAZIA_V   6.0f   // 0%   (3.0V/celula, cutoff seguro)
#define ENERGIA_BAT_CAPACIDADE_MAH   1000   // (a confirmar) capacidade nominal em mAh

// --- Limiares de alerta ------------------------------------------------------
#define ENERGIA_TENSAO_MIN_OP_V      6.5f   // HU21: tensao minima operacional
#define ENERGIA_TENSAO_OSCILACAO_V   0.5f   // HU21: variacao que dispara log de oscilacao
#define ENERGIA_BAT_ALERTA_PCT       20     // HU16: alerta de bateria fraca
#define ENERGIA_BAT_CRITICO_PCT      5      // HU16: alerta critico
#define ENERGIA_CONSUMO_ALERTA_PCT   80     // HU10: alerta ao consumir 80% da capacidade

// --- Cadencia de amostragem --------------------------------------------------
// HU21/HU16/HU10 pedem atualizacao a cada 500 ms.
#define ENERGIA_AMOSTRAGEM_MS        500

// Histerese pra alertas nao piscarem em torno do limiar (em pontos percentuais
// pra bateria, em volts pra tensao).
#define ENERGIA_HISTERESE_PCT        2
#define ENERGIA_HISTERESE_V          0.15f

// Capacidade total da bateria em Wh, derivada da capacidade em mAh e da tensao
// nominal (7.4V). Base pro alerta de 80% de consumo do HU10.
#define ENERGIA_BAT_TENSAO_NOMINAL_V 7.4f
#define ENERGIA_BAT_CAPACIDADE_WH \
    ((ENERGIA_BAT_CAPACIDADE_MAH / 1000.0f) * ENERGIA_BAT_TENSAO_NOMINAL_V)

#endif // ENERGIA_CONFIG_H

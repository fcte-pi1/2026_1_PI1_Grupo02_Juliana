#ifndef ENCODER_H
#define ENCODER_H

#include "pico/stdlib.h"

// --- Parametros do encoder do motor N20 (ASLONG JGA12-N10B) ---
//
// O encoder e magnetico (efeito Hall) com 2 canais em quadratura: C1 e C2.
// Pulsos por volta do EIXO DO MOTOR (antes da reducao). 7 e o valor tipico
// dessa familia de encoder Hall; confirme contando os pulsos de uma volta
// completa do eixo do motor se precisar de precisao.
#define ENCODER_PPR_MOTOR 7

// Decodificacao em quadratura x4: conta as 4 transicoes (bordas) de cada
// ciclo dos canais A/B, quadruplicando a resolucao.
#define ENCODER_QUAD_MULT 4

// Numero maximo de encoders que compartilham a mesma IRQ de GPIO.
// 2 motores (esquerdo/direito) = 2 encoders.
#define ENCODER_MAX 4

typedef struct {
    uint pin_a;                 // canal A (C1 do encoder)
    uint pin_b;                 // canal B (C2 do encoder)
    volatile int32_t count;     // contagem acumulada (x4), com sinal
    volatile uint8_t last_state;// ultimo estado AB (2 bits) p/ decodificacao
    float counts_per_rev;       // contagens por volta do EIXO DE SAIDA
    int32_t last_sample_count;  // apoio para calculo de RPM
    uint64_t last_sample_us;    // apoio para calculo de RPM
} encoder_t;

// Inicializa um encoder. gear_ratio e a reducao da caixa (ex.: 100 para 100:1,
// conforme a coluna "Speed reducer / Ratio" do datasheet). Configura os pinos
// como entrada com pull-up e habilita a IRQ de borda nos dois canais.
void encoder_init(encoder_t *enc, uint pin_a, uint pin_b, uint16_t gear_ratio);

// Zera a contagem e os apoios de calculo de RPM.
void encoder_reset(encoder_t *enc);

// Contagem acumulada em quadratura (com sinal: sentido define +/-).
int32_t encoder_get_count(encoder_t *enc);

// Voltas acumuladas do eixo de saida (count / counts_per_rev).
float encoder_get_revolutions(encoder_t *enc);

// Velocidade do eixo de saida em RPM, calculada a partir da variacao de
// contagem desde a ultima chamada. Chame periodicamente (ex.: a cada 100 ms).
float encoder_get_rpm(encoder_t *enc);

#endif

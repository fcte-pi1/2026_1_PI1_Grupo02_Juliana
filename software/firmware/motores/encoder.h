#ifndef ENCODER_H
#define ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pico/stdlib.h"

// Parâmetros do encoder do motor N20 (ASLONG JGA12-N10B)
// Pulsos por volta do eixo do motor (antes da redução). 7 é o valor típico
// dessa família de encoder Hall; confirme contando os pulsos de uma volta
// completa do eixo do motor se precisar de precisão.
#define ENCODER_PPR_MOTOR 7

// Decodificação em quadratura x4: conta as 4 transições (bordas) de cada
// ciclo dos canais A/B, quadruplicando a resolução.
#define ENCODER_QUAD_MULT 4

// Número máximo de encoders que compartilham a mesma IRQ de GPIO.
// 2 motores (esquerdo/direito) = 2 encoders.
#define ENCODER_MAX 4

typedef struct {
    uint pin_a;                  // canal A (C1 do encoder)
    uint pin_b;                  // canal B (C2 do encoder)
    volatile int32_t count;      // contagem acumulada (x4), com sinal
    volatile uint8_t last_state; // último estado AB (2 bits) p/ decodificação
    float counts_per_rev;        // contagens por volta do eixo de saída
    int32_t last_sample_count;   // apoio para cálculo de RPM
    uint64_t last_sample_us;     // apoio para cálculo de RPM
} encoder_t;

// Inicializa um encoder. gear_ratio é a redução da caixa (ex.: 100 para 100:1,
// conforme a coluna "Speed reducer / Ratio" do datasheet). Configura os pinos
// como entrada com pull-up e habilita a IRQ de borda nos dois canais.
void encoder_init(encoder_t *enc, uint pin_a, uint pin_b, uint16_t gear_ratio);

// Zera a contagem e os apoios de cálculo de RPM.
void encoder_reset(encoder_t *enc);

// Contagem acumulada em quadratura (com sinal: sentido define +/-).
int32_t encoder_get_count(encoder_t *enc);

// Voltas acumuladas do eixo de saída (count / counts_per_rev).
float encoder_get_revolutions(encoder_t *enc);

// Velocidade do eixo de saída em RPM, calculada a partir da variação de
// contagem desde a última chamada. Chame periodicamente (ex.: a cada 100 ms).
float encoder_get_rpm(encoder_t *enc);

#ifdef __cplusplus
}
#endif

#endif

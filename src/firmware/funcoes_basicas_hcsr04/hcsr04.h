#pragma once

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// HC-SR04 — Ultrasonic Ranging Module
// Datasheet specs:
//   - Working voltage:        5 V DC (use VBUS / Pin 40 do Pico W)
//   - Working current:        15 mA
//   - Working frequency:      40 kHz
//   - Range:                  2 cm — 400 cm
//   - Resolution:             ~3 mm
//   - Measuring angle:        15°
//   - Trigger input:          pulso TTL de 10 µs no pino TRIG
//   - Echo output:            pulso TTL proporcional à distância (nível 5 V)
//   - Ciclo recomendado:      ≥ 60 ms entre medições
//   - Fórmula:                distancia_cm = pulso_echo_us / 58
// ---------------------------------------------------------------------------

// Pinagem no Raspberry Pi Pico W
#define HCSR04_TRIG_PIN     16     // GP16 (Pin 21) → TRIG
#define HCSR04_ECHO_PIN     17     // GP17 (Pin 22) ← ECHO (via divisor de tensão 5 V → 3,3 V)

// Parâmetros temporais (datasheet)
#define HCSR04_TRIG_PULSE_US        10      // largura mínima do pulso de trigger
#define HCSR04_SETTLE_MS            50      // tempo de estabilização inicial
#define HCSR04_MIN_CYCLE_MS         60      // intervalo mínimo entre medidas
#define HCSR04_ECHO_TIMEOUT_US      30000   // 30 ms ≈ 5 m (acima do range máximo)

// Faixa válida segundo datasheet
#define HCSR04_MIN_DIST_CM          2.0f
#define HCSR04_MAX_DIST_CM          400.0f

// Constante de conversão: tempo_us / 58 = distância_cm
//   (velocidade do som ≈ 343 m/s → 1 cm round-trip ≈ 58,3 µs)
#define HCSR04_US_PER_CM            58.0f

// Valor retornado quando a leitura é inválida (timeout ou fora de faixa)
#define HCSR04_INVALID_READING      (-1.0f)

// Inicializa os pinos GPIO usados pelo sensor.
void hcsr04_init(void);

// Dispara uma medição bloqueante. Retorna a distância em cm
// ou HCSR04_INVALID_READING em caso de timeout / leitura fora de faixa.
float hcsr04_medir_distancia_cm(void);

// Faz N leituras e retorna a mediana (filtra outliers/ecos espúrios).
// n_amostras deve estar entre 3 e 15.
float hcsr04_medir_filtrado_cm(uint8_t n_amostras);

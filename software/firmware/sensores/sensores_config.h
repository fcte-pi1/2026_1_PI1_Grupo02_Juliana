#ifndef SENSORES_CONFIG_H
#define SENSORES_CONFIG_H

// ============================================================================
// Configuracao dos sensores de distancia (HU04 - medicao de distancia).
// ----------------------------------------------------------------------------
// Alocacao dos 3 sensores (ajustavel conforme a montagem):
//   FRENTE   = VL53L0X no i2c1 (GP6 SDA / GP7 SCL)  - ToF, mede bem < 2cm
//   ESQUERDA = VL53L0X no i2c0 (GP4 SDA / GP5 SCL)  - compartilha o barramento
//              com o MPU6050 (enderecos diferentes: VL53=0x29, MPU=0x68)
//   DIREITA  = HC-SR04 (TRIG GP16 / ECHO GP17)      - ultrassom, piso fisico ~2cm
//
// Os pinos dos motores (GP10-15), do MPU (GP4/GP5) e da energia (GP26/GP27)
// ja estao ocupados; estes pinos foram escolhidos entre os livres.
// ============================================================================

// --- VL53L0X frente (i2c1) ---
#define DIST_VL_FRENTE_I2C     i2c1
#define DIST_VL_FRENTE_SDA     6
#define DIST_VL_FRENTE_SCL     7

// --- VL53L0X esquerda (i2c0, junto do MPU6050) ---
#define DIST_VL_ESQ_I2C        i2c0
#define DIST_VL_ESQ_SDA        4
#define DIST_VL_ESQ_SCL        5

// --- HC-SR04 direita ---
// ECHO opera em 5V: usar divisor resistivo pra 3,3V no pino do Pico.
#define DIST_HC_DIR_TRIG       16
#define DIST_HC_DIR_ECHO       17

// --- Parametros ---
#define DIST_I2C_FREQ_HZ           400000  // 400kHz (fast mode) - mesma freq do MPU6050
#define DIST_VL_TIMING_BUDGET_US   20000   // 20ms/sensor: 2 VL53 + HC-SR04 cabem em <50ms
#define DIST_VL_IO_TIMEOUT_MS      40      // teto de stall por leitura (default do driver ~500ms)
#define DIST_HC_TIMEOUT_US         8000    // ~137cm de eco max; segura o ciclo dentro do orcamento
#define DIST_INTERVALO_MS          50      // HU04: nova leitura a cada 50ms no minimo
#define DIST_COLISAO_CM            2.0f    // HU04: alerta de colisao iminente < 2cm
#define DIST_ALCANCE_UTIL_CM       20.0f   // faixa util de deteccao de parede (criterio: 1-20cm)

// Valor sentinela pra leitura invalida (timeout / fora de range).
#define DIST_INVALIDA_CM          (-1.0f)

#endif // SENSORES_CONFIG_H

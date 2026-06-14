#pragma once
#include <stdint.h>
#include <stdbool.h>

// ──────────────────────────────────────────────────────────────────────────────
// SensorManager — abstração dos sensores de distância
//
// Implementação padrão usa sensores analógicos (IR/Sharp) lidos via ADC nos
// pinos GP26 (frente), GP27 (esquerda), GP28 (direita).
//
// Para sensores I2C (VL53L0X, HCSR04, etc.), implemente os métodos virtuais
// ou substitua readRaw() conforme o driver do sensor.
// ──────────────────────────────────────────────────────────────────────────────

// Distância mínima em "unidades raw" abaixo da qual considera-se parede presente.
// DEVE ser calibrado para o sensor e geometria do robô.
// Para sensores Sharp GP2Y0A21 em uma célula de 18 cm, ~2200 ADC é um ponto de partida.
#define SENSOR_WALL_THRESHOLD_RAW  2200u

// Pinos ADC padrão (GP26=ADC0, GP27=ADC1, GP28=ADC2)
#define SENSOR_PIN_FRONT  26u
#define SENSOR_PIN_LEFT   27u
#define SENSOR_PIN_RIGHT  28u

class SensorManager {
public:
    SensorManager(uint32_t pin_front = SENSOR_PIN_FRONT,
                  uint32_t pin_left  = SENSOR_PIN_LEFT,
                  uint32_t pin_right = SENSOR_PIN_RIGHT);

    // Inicializa ADC e pinos. Chamar uma vez no setup.
    void init();

    // Lê todos os sensores. Chamar antes de hasWallXxx().
    void update();

    bool hasWallFront() const;
    bool hasWallLeft()  const;
    bool hasWallRight() const;

    // Valores raw do ADC (0..4095) — úteis para calibração e telemetria.
    uint16_t rawFront() const;
    uint16_t rawLeft()  const;
    uint16_t rawRight() const;

    // Ajusta o limiar em tempo de execução (ex: calibração automática).
    void setThreshold(uint16_t threshold);

private:
    uint32_t pin_front_, pin_left_, pin_right_;
    uint16_t raw_front_, raw_left_, raw_right_;
    uint16_t threshold_;

    uint16_t readAdc(uint32_t gpio) const;
};

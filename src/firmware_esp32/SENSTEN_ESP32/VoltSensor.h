#ifndef VOLT_SENSOR_H
#define VOLT_SENSOR_H

#include <Arduino.h>

typedef struct {
    uint8_t gpio;
    float divider_ratio;
} voltage_sensor_t;

// Inicializa o sensor (configura o pino como entrada analogica)
void voltage_sensor_init(voltage_sensor_t *sensor, uint8_t gpio, float divider_ratio);

// Retorna a leitura crua do ADC (0-4095)
uint16_t voltage_sensor_read_raw(voltage_sensor_t *sensor);

// Retorna a tensao de entrada em Volts
float voltage_sensor_read_v(voltage_sensor_t *sensor);

#endif

#ifndef VOLTAGE_SENSOR_H
#define VOLTAGE_SENSOR_H

#include "pico/stdlib.h"

typedef struct {
    uint gpio;
    uint adc_channel;
    float divider_ratio;
} voltage_sensor_t;

// Inicializa o ADC e configura o pino do sensor
void voltage_sensor_init(voltage_sensor_t *sensor, uint gpio, float divider_ratio);

// Retorna a leitura crua do ADC (0-4095), sem nenhuma conversao
uint16_t voltage_sensor_read_raw(voltage_sensor_t *sensor);

// Retorna a tensao de entrada em Volts
float voltage_sensor_read_v(voltage_sensor_t *sensor);

#endif
#include "VoltSensor.h"

#define ADC_VREF  3.3f
#define ADC_MAX   4095.0f
#define N_SAMPLES 16

void voltage_sensor_init(voltage_sensor_t *sensor, uint8_t gpio, float divider_ratio) {
    sensor->gpio = gpio;
    sensor->divider_ratio = divider_ratio;
    pinMode(gpio, INPUT);
}

uint16_t voltage_sensor_read_raw(voltage_sensor_t *sensor) {
    uint32_t soma = 0;
    for (int i = 0; i < N_SAMPLES; i++) {
        soma += analogRead(sensor->gpio);
        delayMicroseconds(100);
    }
    return (uint16_t)(soma / N_SAMPLES);
}

float voltage_sensor_read_v(voltage_sensor_t *sensor) {
    float media = (float)voltage_sensor_read_raw(sensor);
    float v_pino = media * (ADC_VREF / ADC_MAX);
    return v_pino * sensor->divider_ratio;
}

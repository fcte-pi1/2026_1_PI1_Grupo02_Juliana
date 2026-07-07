#include "current_sensor.h"

static const int N_AMOSTRAS = 16;

static uint16_t read_raw_avg(current_sensor_t *sensor) {
    // Media de N leituras para reduzir ruido
    uint32_t soma = 0;
    for (int i = 0; i < N_AMOSTRAS; i++) {
        soma += analogRead(sensor->pin);
        delayMicroseconds(100);
    }
    return (uint16_t)(soma / N_AMOSTRAS);
}

static uint32_t read_mv_avg(current_sensor_t *sensor) {
    // analogReadMilliVolts usa a calibracao de fabrica (eFuse) do ESP32,
    // que e bem mais precisa que converter o valor cru manualmente.
    uint32_t soma = 0;
    for (int i = 0; i < N_AMOSTRAS; i++) {
        soma += analogReadMilliVolts(sensor->pin);
        delayMicroseconds(100);
    }
    return soma / N_AMOSTRAS;
}

void current_sensor_init(current_sensor_t *sensor, uint8_t pin, float sensitivity_mv_per_a, uint32_t zero_offset_mv) {
    sensor->pin = pin;
    sensor->sensitivity_mv_per_a = sensitivity_mv_per_a;
    sensor->zero_offset_mv = zero_offset_mv;

    analogReadResolution(12);                    // 0-4095
    analogSetPinAttenuation(pin, ADC_11db);       // permite ler ate ~3.3V no pino
    pinMode(pin, INPUT);
}

void current_sensor_calibrate_zero(current_sensor_t *sensor) {
    sensor->zero_offset_mv = read_mv_avg(sensor);
}

uint16_t current_sensor_read_raw(current_sensor_t *sensor) {
    return read_raw_avg(sensor);
}

uint32_t current_sensor_read_output_mv(current_sensor_t *sensor) {
    return read_mv_avg(sensor);
}

int32_t current_sensor_read_ma(current_sensor_t *sensor) {
    uint32_t out_mv = current_sensor_read_output_mv(sensor);
    int32_t delta_mv = (int32_t)out_mv - (int32_t)sensor->zero_offset_mv;

    // mA = (delta_mV / sensibilidade_mV_por_A) * 1000
    return (int32_t)((delta_mv * 1000.0f) / sensor->sensitivity_mv_per_a);
}

float current_sensor_read_a(current_sensor_t *sensor) {
    return current_sensor_read_ma(sensor) / 1000.0f;
}

#include "voltage_sensor.h"
#include "hardware/adc.h"

#define ADC_VREF_MV 3300u   // tensao de referencia do ADC em mV (3.3V)
#define ADC_MAX     4095u   // valor maximo do ADC de 12 bits

// Mapeia GPIO -> canal ADC (RP2040: GPIO26=0, 27=1, 28=2, 29=3)
static uint gpio_to_adc_channel(uint gpio) {
    return gpio - 26;
}

void voltage_sensor_init(voltage_sensor_t *sensor, uint gpio, float divider_ratio) {
    sensor->gpio = gpio;
    sensor->adc_channel = gpio_to_adc_channel(gpio);
    sensor->divider_ratio = divider_ratio;

    adc_init();
    adc_gpio_init(gpio);
}

uint32_t voltage_sensor_read_mv(voltage_sensor_t *sensor) {
    adc_select_input(sensor->adc_channel);

    // Media de N leituras para reduzir ruido
    const int N = 16;
    uint32_t soma = 0;
    for (int i = 0; i < N; i++) {
        soma += adc_read();
        sleep_us(100);
    }
    uint32_t media_raw = soma / N; // contagem 0-4095

    // Passo 1: contagem ADC -> milivolts no PINO do microcontrolador
    uint32_t mv_no_pino = (media_raw * ADC_VREF_MV) / ADC_MAX;

    // Passo 2: aplica o divisor resistivo do modulo -> mV na ENTRADA real
    uint32_t mv_entrada = (uint32_t)(mv_no_pino * sensor->divider_ratio);

    return mv_entrada;
}

uint16_t voltage_sensor_read_raw(voltage_sensor_t *sensor) {
    adc_select_input(sensor->adc_channel);

    const int N = 16;
    uint32_t soma = 0;
    for (int i = 0; i < N; i++) {
        soma += adc_read();
        sleep_us(100);
    }
    return (uint16_t)(soma / N);
}

float voltage_sensor_read_v(voltage_sensor_t *sensor) {
    return voltage_sensor_read_mv(sensor) / 1000.0f;
}
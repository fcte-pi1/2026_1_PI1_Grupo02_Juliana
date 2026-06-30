#include "current_sensor.h"
#include "hardware/adc.h"

#define ADC_VREF_MV 3300u   // tensao de referencia do ADC em mV (3.3V)
#define ADC_MAX     4095u   // valor maximo do ADC de 12 bits

// Mapeia GPIO -> canal ADC (RP2040: GPIO26=0, 27=1, 28=2, 29=3)
static uint gpio_to_adc_channel(uint gpio) {
    return gpio - 26;
}

static uint16_t read_raw_avg(current_sensor_t *sensor) {
    adc_select_input(sensor->adc_channel);

    // Media de N leituras para reduzir ruido
    const int N = 16;
    uint32_t soma = 0;
    for (int i = 0; i < N; i++) {
        soma += adc_read();
        sleep_us(100);
    }
    return (uint16_t)(soma / N);
}

void current_sensor_init(current_sensor_t *sensor, uint gpio, float sensitivity_mv_per_a, uint32_t zero_offset_mv) {
    sensor->gpio = gpio;
    sensor->adc_channel = gpio_to_adc_channel(gpio);
    sensor->sensitivity_mv_per_a = sensitivity_mv_per_a;
    sensor->zero_offset_mv = zero_offset_mv;

    adc_init();
    adc_gpio_init(gpio);
}

void current_sensor_calibrate_zero(current_sensor_t *sensor) {
    uint16_t raw = read_raw_avg(sensor);
    sensor->zero_offset_mv = (raw * ADC_VREF_MV) / ADC_MAX;
}

uint16_t current_sensor_read_raw(current_sensor_t *sensor) {
    return read_raw_avg(sensor);
}

uint32_t current_sensor_read_output_mv(current_sensor_t *sensor) {
    uint16_t raw = read_raw_avg(sensor);
    return (raw * ADC_VREF_MV) / ADC_MAX;
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

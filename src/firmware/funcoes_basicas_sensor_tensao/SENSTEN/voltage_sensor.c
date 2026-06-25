#include "voltage_sensor.h"
#include "hardware/adc.h"

#define ADC_VREF  3.3f
#define ADC_MAX   4095.0f

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
    adc_select_input(sensor->adc_channel);

    const int N = 16;
    uint32_t soma = 0;
    for (int i = 0; i < N; i++) {
        soma += adc_read();
        sleep_us(100);
    }
    float media = (float)(soma / N);

    // Converte leitura ADC para volts no pino, depois multiplica pelo divisor (x5)
    float v_pino = media * (ADC_VREF / ADC_MAX);
    return v_pino * sensor->divider_ratio;
}
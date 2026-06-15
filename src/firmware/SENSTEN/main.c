#include <stdio.h>
#include "pico/stdlib.h"
#include "voltage_sensor.h"

int main(void) {
    stdio_init_all();

    voltage_sensor_t v_sensor;
    voltage_sensor_init(&v_sensor, 26, 5.0f); // GPIO26 (ADC0), fator 5:1 calibrado

    while (true) {
        uint32_t tensao_mv = voltage_sensor_read_mv(&v_sensor);
        float tensao_v = tensao_mv / 1000.0f;

        printf("Tensao: %lu mV (%.2f V)\n", tensao_mv, tensao_v);

        sleep_ms(500);
    }
}
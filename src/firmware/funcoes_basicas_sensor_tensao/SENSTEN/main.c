#include <stdio.h>
#include "pico/stdlib.h"
#include "voltage_sensor.h"

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // aguarda monitor serial conectar

    voltage_sensor_t v_sensor;
    voltage_sensor_init(&v_sensor, 26, 5.0f); // GPIO26 (ADC0), fator 5:1 calibrado

    while (true) {
        float tensao = voltage_sensor_read_v(&v_sensor);
        printf("Tensao: %.2f V\n", tensao);
        sleep_ms(500);
    }
}
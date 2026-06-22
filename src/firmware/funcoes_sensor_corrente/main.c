#include <stdio.h>
#include "pico/stdlib.h"
#include "current_sensor.h"

int main(void) {
    stdio_init_all();

    current_sensor_t i_sensor;
    // GPIO27 (ADC1), jumper do modulo em 5A -> 185 mV/A, zero em VCC/2 (~1650 mV p/ 3.3V)
    current_sensor_init(&i_sensor, 27, CURRENT_SENSOR_SENS_5A, 1650);

    while (true) {
        int32_t corrente_ma = current_sensor_read_ma(&i_sensor);
        float corrente_a = corrente_ma / 1000.0f;

        printf("Corrente: %ld mA (%.2f A)\n", corrente_ma, corrente_a);

        sleep_ms(500);
    }
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "voltage_sensor.h"

#define ADC_MAX  4095u
#define ADC_VREF 3.3f

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // aguarda monitor serial conectar

    voltage_sensor_t v_sensor;
    voltage_sensor_init(&v_sensor, 26, 5.0f); // GPIO26 (ADC0), fator 5:1 calibrado

    printf("=== TESTE/VALIDACAO - Sensor de Tensao FZ0430 ===\n\n");

    while (true) {
        // Leitura crua do ADC
        uint16_t raw = voltage_sensor_read_raw(&v_sensor);

        // Tensao no pino S (antes do divisor)
        float v_pino = raw * (ADC_VREF / ADC_MAX);

        // Tensao na entrada (apos multiplicar por 5)
        float v_entrada = voltage_sensor_read_v(&v_sensor);

        printf("RAW ADC       : %4u / %u\n", raw, ADC_MAX);
        printf("Tensao no pino: %.3f V\n", v_pino);
        printf("Tensao entrada: %.3f V\n", v_entrada);

        // Sanity checks
        if (raw == 0) {
            printf("[AVISO] RAW = 0 -> verifique GND comum ou pino S desconectado\n");
        }
        if (raw >= ADC_MAX) {
            printf("[AVISO] RAW no maximo -> possivel saturacao (entrada > 16.5V?)\n");
        }
        if (v_entrada > 25.0f) {
            printf("[ERRO] Tensao acima do limite do modulo (VCC<25V)!\n");
        }

        printf("--------------------------------------------\n");
        sleep_ms(1000);
    }
}
#include <stdio.h>
#include "pico/stdlib.h"
#include "voltage_sensor.h"

#define ADC_MAX     4095u
#define ADC_VREF_MV 3300u

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // aguarda monitor serial conectar

    voltage_sensor_t v_sensor;
    voltage_sensor_init(&v_sensor, 26, 5.0f); // ajuste o fator apos calibracao

    printf("=== TESTE/VALIDACAO - Sensor de Tensao FZ0430 ===\n\n");

    while (true) {
        // --- Leitura crua do ADC ---
        uint16_t raw = voltage_sensor_read_raw(&v_sensor);

        // --- Conversao para mV no pino ---
        uint32_t mv_pino = (raw * ADC_VREF_MV) / ADC_MAX;

        // --- Conversao final (apos divisor) ---
        uint32_t mv_entrada = voltage_sensor_read_mv(&v_sensor);
        float v_entrada = mv_entrada / 1000.0f;

        printf("RAW ADC: %4u / %u\n", raw, ADC_MAX);
        printf("Tensao no pino S : %4lu mV\n", mv_pino);
        printf("Tensao na entrada: %5lu mV  (%.3f V)\n", mv_entrada, v_entrada);

        // --- Validacoes automaticas (sanity checks) ---
        if (raw == 0) {
            printf("[AVISO] RAW = 0 -> verifique GND comum ou pino S desconectado\n");
        }
        if (raw >= ADC_MAX) {
            printf("[AVISO] RAW no maximo -> possivel saturacao (entrada > 16.5V?)\n");
        }
        if (mv_entrada > 25000) {
            printf("[ERRO] Tensao acima do limite do modulo (VCC<25V)!\n");
        }

        printf("--------------------------------------------\n");
        sleep_ms(1000);
    }
}
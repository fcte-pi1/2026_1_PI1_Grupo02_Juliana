#include <stdio.h>
#include "pico/stdlib.h"
#include "current_sensor.h"

#define ADC_MAX     4095u
#define ADC_VREF_MV 3300u

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // aguarda monitor serial conectar

    current_sensor_t i_sensor;
    // GPIO27 (ADC1), jumper do modulo em 5A -> 185 mV/A; zero inicial em VCC/2 (ajustado abaixo)
    current_sensor_init(&i_sensor, 27, CURRENT_SENSOR_SENS_5A, 2500);

    printf("=== TESTE/VALIDACAO - Sensor de Corrente HW-872 (efeito Hall) ===\n\n");

    printf("Calibrando zero - mantenha IP+/IP- sem corrente...\n");
    sleep_ms(500);
    current_sensor_calibrate_zero(&i_sensor);
    printf("Zero calibrado: %lu mV\n\n", i_sensor.zero_offset_mv);

    while (true) {
        // --- Leitura crua do ADC ---
        uint16_t raw = current_sensor_read_raw(&i_sensor);

        // --- Tensao de saida (pino OUT) ---
        uint32_t out_mv = current_sensor_read_output_mv(&i_sensor);

        // --- Conversao final para corrente ---
        int32_t corrente_ma = current_sensor_read_ma(&i_sensor);
        float corrente_a = corrente_ma / 1000.0f;

        printf("RAW ADC: %4u / %u\n", raw, ADC_MAX);
        printf("Tensao de saida (OUT): %4lu mV\n", out_mv);
        printf("Corrente: %6ld mA  (%.3f A)\n", corrente_ma, corrente_a);

        // --- Validacoes automaticas (sanity checks) ---
        if (raw == 0) {
            printf("[AVISO] RAW = 0 -> verifique GND comum ou pino OUT desconectado\n");
        }
        if (raw >= ADC_MAX) {
            printf("[AVISO] RAW no maximo -> possivel saturacao (corrente acima do range selecionado?)\n");
        }

        printf("--------------------------------------------\n");
        sleep_ms(1000);
    }
}

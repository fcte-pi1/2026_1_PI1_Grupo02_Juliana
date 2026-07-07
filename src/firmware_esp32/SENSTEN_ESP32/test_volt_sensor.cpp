#include <Arduino.h>
#include "VoltSensor.h"

#define ADC_MAX  4095u
#define ADC_VREF 3.3f

voltage_sensor_t v_sensor;

void setup() {
    Serial.begin(115200);
    delay(2000);

    voltage_sensor_init(&v_sensor, 34, 5.0f);

    Serial.println("=== TESTE/VALIDACAO - Sensor de Tensao FZ0430 ===\n");
}

void loop() {
    uint16_t raw    = voltage_sensor_read_raw(&v_sensor);
    float v_pino    = raw * (ADC_VREF / ADC_MAX);
    float v_entrada = voltage_sensor_read_v(&v_sensor);

    Serial.printf("RAW ADC       : %4u / %u\n", raw, ADC_MAX);
    Serial.printf("Tensao no pino: %.3f V\n", v_pino);
    Serial.printf("Tensao entrada: %.3f V\n", v_entrada);

    if (raw == 0)
        Serial.println("[AVISO] RAW = 0 -> verifique GND comum ou pino desconectado");
    if (raw >= ADC_MAX)
        Serial.println("[AVISO] RAW no maximo -> possivel saturacao (entrada > 16.5V?)");
    if (v_entrada > 25.0f)
        Serial.println("[ERRO] Tensao acima do limite do modulo (VCC<25V)!");

    Serial.println("--------------------------------------------");
    delay(1000);
}

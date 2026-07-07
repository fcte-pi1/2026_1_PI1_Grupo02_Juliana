#include <Arduino.h>
#include "VoltSensor.h"

voltage_sensor_t v_sensor;

void setup() {
    Serial.begin(115200);
    delay(2000); // aguarda monitor serial conectar

    voltage_sensor_init(&v_sensor, 34, 5.0f); // GPIO34 (ADC), fator 5:1
}

void loop() {
    float tensao = voltage_sensor_read_v(&v_sensor);
    Serial.printf("Tensao: %.2f V\n", tensao);
    delay(500);
}

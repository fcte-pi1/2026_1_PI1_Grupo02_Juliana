#include "current_sensor.h"

current_sensor_t i_sensor;

void setup() {
    Serial.begin(115200);
    delay(2000); // aguarda monitor serial conectar

    // GPIO34 (ADC1_CH6, so entrada), jumper do modulo em 5A -> 185 mV/A
    current_sensor_init(&i_sensor, 34, CURRENT_SENSOR_SENS_5A, 1650);

    Serial.println("Calibrando zero - mantenha IP+/IP- sem corrente...");
    delay(500);
    current_sensor_calibrate_zero(&i_sensor);
    Serial.printf("Zero calibrado: %lu mV\n\n", i_sensor.zero_offset_mv);
}

void loop() {
    int32_t corrente_ma = current_sensor_read_ma(&i_sensor);
    float corrente_a = corrente_ma / 1000.0f;

    Serial.printf("Corrente: %ld mA (%.2f A)\n", corrente_ma, corrente_a);

    delay(500);
}

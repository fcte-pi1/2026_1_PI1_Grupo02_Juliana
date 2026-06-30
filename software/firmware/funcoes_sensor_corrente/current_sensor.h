#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

#include "pico/stdlib.h"

// Sensibilidades tipicas (mV por Ampere) para cada posicao do jumper de range
// do modulo de corrente por efeito Hall HW-872A/B/C.
// Sao valores de referencia (mesma familia de chip do ACS712) - calibre o
// sensor com uma carga de corrente conhecida e ajuste sensitivity_mv_per_a
// se a leitura nao corresponder ao valor real.
#define CURRENT_SENSOR_SENS_5A   185.0f
#define CURRENT_SENSOR_SENS_10A  100.0f
#define CURRENT_SENSOR_SENS_20A   66.0f
#define CURRENT_SENSOR_SENS_30A   40.0f

typedef struct {
    uint gpio;
    uint adc_channel;
    float sensitivity_mv_per_a;  // mV por Ampere (depende do range selecionado no modulo)
    uint32_t zero_offset_mv;     // tensao de saida (OUT) com corrente = 0A, tipicamente VCC/2
} current_sensor_t;

// Inicializa o ADC e configura o pino do sensor (pino OUT do modulo)
void current_sensor_init(current_sensor_t *sensor, uint gpio, float sensitivity_mv_per_a, uint32_t zero_offset_mv);

// Mede a tensao de saida em repouso (IP+/IP- sem corrente) e atualiza zero_offset_mv.
// Chamar com a carga de potencia desligada/desconectada.
void current_sensor_calibrate_zero(current_sensor_t *sensor);

// Retorna a corrente medida em miliamperes (sinal indica o sentido do fluxo)
int32_t current_sensor_read_ma(current_sensor_t *sensor);

// Retorna a corrente medida em Amperes (conveniencia, baseado em read_ma)
float current_sensor_read_a(current_sensor_t *sensor);

// Retorna a leitura crua do ADC (0-4095), sem nenhuma conversao
uint16_t current_sensor_read_raw(current_sensor_t *sensor);

// Retorna a tensao de saida do sensor no pino OUT, em milivolts
uint32_t current_sensor_read_output_mv(current_sensor_t *sensor);

#endif

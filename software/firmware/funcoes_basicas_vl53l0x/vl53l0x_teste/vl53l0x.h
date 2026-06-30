#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// Endereço padrão de fábrica (7 bits) do VL53L0X
#define VL53L0X_DEFAULT_ADDRESS 0x29

// Tipo de período VCSEL (usado nas funções de timing budget)
typedef enum {
    VcselPeriodPreRange,
    VcselPeriodFinalRange
} vcselPeriodType;

// Estado de um sensor. Cada sensor físico tem a sua própria struct.
typedef struct {
    i2c_inst_t *i2c_port;                  // i2c0 ou i2c1
    uint8_t     address;                   // endereço I2C atual (7 bits)
    uint16_t    io_timeout_ms;             // timeout de I/O em ms (0 = desativado)
    bool        did_timeout;               // true se a última leitura deu timeout
    uint16_t    timeout_start_ms;          // uso interno
    uint8_t     stop_variable;             // "stop variable" lida na init
    uint32_t    measurement_timing_budget_us;
} VL53L0X;

// ------------------------------------------------------------------
// API pública
// ------------------------------------------------------------------

// Inicialização completa. Deve ser chamada uma vez por sensor, depois
// de o barramento I2C correspondente já estar configurado.
// io_2v8 = true para plaquinhas breakout que operam o I/O em 2.8V (padrão).
// Retorna false se a comunicação ou a calibração falharem.
bool vl53l0x_init(VL53L0X *sensor, i2c_inst_t *i2c_port, bool io_2v8);

// (Opcional) Troca o endereço I2C do sensor. Útil para vários sensores
// no MESMO barramento. Não é necessário se cada sensor está num
// barramento separado (i2c0/i2c1).
void vl53l0x_set_address(VL53L0X *sensor, uint8_t new_address);

// Lê o ID do modelo (registo 0xC0). Deve retornar 0xEE num VL53L0X.
// Bom para diagnosticar fiação/endereço antes de confiar nas medições.
uint8_t vl53l0x_read_model_id(VL53L0X *sensor);

// Define o timing budget (em microssegundos). Maior = mais preciso/lento.
// Mínimo ~20000. Default da init = 33000.
bool vl53l0x_set_measurement_timing_budget(VL53L0X *sensor, uint32_t budget_us);
uint32_t vl53l0x_get_measurement_timing_budget(VL53L0X *sensor);

// Faz UMA medição single-shot e retorna a distância em milímetros.
// Retorna 65535 em caso de timeout (verifique sensor->did_timeout).
uint16_t vl53l0x_read_range_single_millimeters(VL53L0X *sensor);

#endif // VL53L0X_H

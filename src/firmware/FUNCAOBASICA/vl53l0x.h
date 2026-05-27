#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// Endereço padrão
#define VL53L0X_DEFAULT_ADDRESS 0x29

// Registradores Críticos
#define SYSRANGE_START                              0x00
#define RESULT_RANGE_STATUS                         0x14
#define SYSTEM_INTERRUPT_CLEAR                      0x0B
#define VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           0x89
#define MSRC_CONFIG_CONTROL                         0x60
#define FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define SYSTEM_SEQUENCE_CONFIG                      0x01

typedef struct {
    i2c_inst_t *i2c_port;
    uint8_t address;
    uint32_t io_timeout_ms;
} VL53L0X;

// Funções da API
bool vl53l0x_init(VL53L0X *sensor, i2c_inst_t *i2c_port);
uint16_t vl53l0x_read_range_single_millimeters(VL53L0X *sensor);

#endif
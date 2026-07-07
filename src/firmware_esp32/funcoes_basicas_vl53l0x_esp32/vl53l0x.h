#pragma once

#include <Arduino.h>
#include <Wire.h>

#define VL53L0X_DEFAULT_ADDRESS 0x29

typedef enum {
    VcselPeriodPreRange,
    VcselPeriodFinalRange
} vcselPeriodType;

typedef struct {
    TwoWire *i2c_port;
    uint8_t  address;
    uint16_t io_timeout_ms;
    bool     did_timeout;
    uint32_t timeout_start_ms;
    uint8_t  stop_variable;
    uint32_t measurement_timing_budget_us;
} VL53L0X;

bool     vl53l0x_init(VL53L0X *sensor, TwoWire *i2c_port, bool io_2v8);
void     vl53l0x_set_address(VL53L0X *sensor, uint8_t new_address);
uint8_t  vl53l0x_read_model_id(VL53L0X *sensor);
bool     vl53l0x_set_measurement_timing_budget(VL53L0X *sensor, uint32_t budget_us);
uint32_t vl53l0x_get_measurement_timing_budget(VL53L0X *sensor);
uint16_t vl53l0x_read_range_single_millimeters(VL53L0X *sensor);

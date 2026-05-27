#include "vl53l0x.h"
#include <pico/stdlib.h>

static void write_reg(VL53L0X *sensor, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(sensor->i2c_port, sensor->address, buffer, 2, false);
}

static uint8_t read_reg(VL53L0X *sensor, uint8_t reg) {
    uint8_t value = 0;
    i2c_write_blocking(sensor->i2c_port, sensor->address, &reg, 1, true);
    i2c_read_blocking(sensor->i2c_port, sensor->address, &value, 1, false);
    return value;
}

static uint16_t read_reg16(VL53L0X *sensor, uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(sensor->i2c_port, sensor->address, &reg, 1, true);
    i2c_read_blocking(sensor->i2c_port, sensor->address, buffer, 2, false);
    return (uint16_t)((buffer[0] << 8) | buffer[1]);
}

bool vl53l0x_init(VL53L0X *sensor, i2c_inst_t *i2c_port) {
    sensor->i2c_port = i2c_port;
    sensor->address = VL53L0X_DEFAULT_ADDRESS;
    sensor->io_timeout_ms = 500; 

    uint8_t vhv_config = read_reg(sensor, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV);
    write_reg(sensor, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, vhv_config | 0x01);

    write_reg(sensor, 0x88, 0x00);
    write_reg(sensor, 0x80, 0x01);
    write_reg(sensor, 0xFF, 0x01);
    write_reg(sensor, 0x00, 0x00);
    write_reg(sensor, 0x91, 0x3c);
    write_reg(sensor, 0x00, 0x01);
    write_reg(sensor, 0xFF, 0x00);
    write_reg(sensor, 0x80, 0x00);

    write_reg(sensor, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 0x00);
    write_reg(sensor, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT + 1, 0x80);

    write_reg(sensor, SYSTEM_SEQUENCE_CONFIG, 0xFF);

    return true; 
}

uint16_t vl53l0x_read_range_single_millimeters(VL53L0X *sensor) {
    write_reg(sensor, 0x80, 0x01);
    write_reg(sensor, 0xFF, 0x01);
    write_reg(sensor, 0x00, 0x00);
    write_reg(sensor, 0x91, 0x3c);
    write_reg(sensor, 0x00, 0x01);
    write_reg(sensor, 0xFF, 0x00);
    write_reg(sensor, 0x80, 0x00);
    write_reg(sensor, SYSRANGE_START, 0x01);

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while ((read_reg(sensor, RESULT_RANGE_STATUS) & 0x01) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start_time) > sensor->io_timeout_ms) {
            return 8190;
        }
        sleep_us(500);
    }

    uint16_t range_mm = read_reg16(sensor, RESULT_RANGE_STATUS + 10);

    write_reg(sensor, SYSTEM_INTERRUPT_CLEAR, 0x01);

    return range_mm;
}
#pragma once

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdbool.h>

// MPU-6050 I2C address (AD0 = GND → 0x68)
#define MPU6050_ADDR        0x68

// --- Register map (datasheet §4) ---
#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C
#define REG_INT_ENABLE      0x38
#define REG_GYRO_XOUT_H     0x43
#define REG_GYRO_XOUT_L     0x44
#define REG_GYRO_YOUT_H     0x45
#define REG_GYRO_YOUT_L     0x46
#define REG_GYRO_ZOUT_H     0x47
#define REG_GYRO_ZOUT_L     0x48
#define REG_PWR_MGMT_1      0x6B
#define REG_WHO_AM_I        0x75

// GYRO_CONFIG FS_SEL bits → full-scale range and LSB sensitivity (datasheet §4.4)
#define GYRO_FS_250DPS      0x00   // ±250 °/s  → 131 LSB/(°/s)
#define GYRO_FS_500DPS      0x08   // ±500 °/s  →  65.5 LSB/(°/s)
#define GYRO_FS_1000DPS     0x10   // ±1000 °/s →  32.8 LSB/(°/s)
#define GYRO_FS_2000DPS     0x18   // ±2000 °/s →  16.4 LSB/(°/s)

// LSB sensitivities (°/s per LSB count)
#define GYRO_SENS_250DPS    131.0f
#define GYRO_SENS_500DPS    65.5f
#define GYRO_SENS_1000DPS   32.8f
#define GYRO_SENS_2000DPS   16.4f

// Pico W I2C bus and pins used for the MPU-6050
#define MPU6050_I2C_PORT    i2c0
#define MPU6050_SDA_PIN     4      // GP4
#define MPU6050_SCL_PIN     5      // GP5
#define MPU6050_I2C_FREQ    400000 // 400 kHz fast mode

// Number of samples for gyroscope calibration
#define GYRO_CALIB_SAMPLES  500

typedef struct {
    float x;   // °/s
    float y;
    float z;
} GyroData;

typedef struct {
    float x;   // bias offset in °/s
    float y;
    float z;
} GyroBias;

typedef struct {
    float yaw;    // °  (rotation around Z-axis, used for heading)
    float pitch;  // °  (rotation around Y-axis)
    float roll;   // °  (rotation around X-axis)
} Orientation;

// Initialise I2C and the MPU-6050. Returns true on success.
bool mpu6050_init(void);

// Read raw gyroscope data converted to °/s.
bool mpu6050_read_gyro(GyroData *out);

// Compute gyroscope bias by averaging GYRO_CALIB_SAMPLES readings at rest.
void mpu6050_calibrate(GyroBias *bias);

// Integrate gyroscope data over dt (seconds) and update orientation.
void mpu6050_update_orientation(const GyroData *gyro,
                                const GyroBias *bias,
                                Orientation *orientation,
                                float dt);

// Reset orientation to zero.
void mpu6050_reset_orientation(Orientation *orientation);

// Normalise a heading angle to [0, 360).
float mpu6050_normalise_heading(float heading_deg);

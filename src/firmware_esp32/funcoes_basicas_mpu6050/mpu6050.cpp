#include "mpu6050.h"
#include <Wire.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static float gyro_sensitivity = GYRO_SENS_250DPS;

static bool reg_write(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static bool reg_read(uint8_t reg, uint8_t *dst, size_t len)
{
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false; // repeated start

    uint8_t received = Wire.requestFrom((int)MPU6050_ADDR, (int)len, (int)true);
    if (received != len) return false;

    for (size_t i = 0; i < len; i++) {
        dst[i] = Wire.read();
    }
    return true;
}

static int16_t raw16(uint8_t high, uint8_t low)
{
    return (int16_t)((high << 8) | low);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool mpu6050_init(void)
{
    Wire.begin(MPU6050_SDA_PIN, MPU6050_SCL_PIN, MPU6050_I2C_FREQ);

    delay(100); // wait for MPU-6050 power-on reset

    // WHO_AM_I must return 0x68 (datasheet section 4.28)
    uint8_t who_am_i = 0;
    if (!reg_read(REG_WHO_AM_I, &who_am_i, 1) || who_am_i != 0x68) {
        Serial.printf("[MPU6050] WHO_AM_I failed: 0x%02X\n", who_am_i);
        return false;
    }

    // Wake up device (clear sleep bit in PWR_MGMT_1)
    if (!reg_write(REG_PWR_MGMT_1, 0x00)) return false;
    delay(10);

    // Sample rate = Gyro output rate / (1 + SMPLRT_DIV)
    // 1 kHz / (1 + 7) = 125 Hz
    if (!reg_write(REG_SMPLRT_DIV, 0x07)) return false;

    // DLPF_CFG = 3 -> bandwidth 42 Hz, delay 4.8 ms (datasheet section 4.3)
    if (!reg_write(REG_CONFIG, 0x03)) return false;

    // Gyroscope full scale +-250 deg/s (FS_SEL = 0, datasheet section 4.4)
    if (!reg_write(REG_GYRO_CONFIG, GYRO_FS_250DPS)) return false;
    gyro_sensitivity = GYRO_SENS_250DPS;

    Serial.printf("[MPU6050] Initialised OK (WHO_AM_I=0x%02X)\n", who_am_i);
    return true;
}

bool mpu6050_read_gyro(GyroData *out)
{
    uint8_t buf[6];
    if (!reg_read(REG_GYRO_XOUT_H, buf, 6)) return false;

    int16_t rx = raw16(buf[0], buf[1]);
    int16_t ry = raw16(buf[2], buf[3]);
    int16_t rz = raw16(buf[4], buf[5]);

    out->x = (float)rx / gyro_sensitivity;
    out->y = (float)ry / gyro_sensitivity;
    out->z = (float)rz / gyro_sensitivity;
    return true;
}

void mpu6050_calibrate(GyroBias *bias)
{
    Serial.printf("[MPU6050] Calibrating gyroscope (%d samples)...\n", GYRO_CALIB_SAMPLES);
    double sx = 0.0, sy = 0.0, sz = 0.0;
    GyroData sample;
    int count = 0;

    for (int i = 0; i < GYRO_CALIB_SAMPLES; i++) {
        if (mpu6050_read_gyro(&sample)) {
            sx += sample.x;
            sy += sample.y;
            sz += sample.z;
            count++;
        }
        delay(4); // ~250 Hz, above the 125 Hz sample rate
    }

    if (count > 0) {
        bias->x = (float)(sx / count);
        bias->y = (float)(sy / count);
        bias->z = (float)(sz / count);
    } else {
        bias->x = bias->y = bias->z = 0.0f;
    }

    Serial.printf("[MPU6050] Bias: X=%.4f  Y=%.4f  Z=%.4f deg/s\n",
                  bias->x, bias->y, bias->z);
}

void mpu6050_update_orientation(const GyroData *gyro,
                                const GyroBias *bias,
                                Orientation *orientation,
                                float dt)
{
    orientation->roll  += (gyro->x - bias->x) * dt;
    orientation->pitch += (gyro->y - bias->y) * dt;
    orientation->yaw   -= (gyro->z - bias->z) * dt;
}

void mpu6050_reset_orientation(Orientation *orientation)
{
    orientation->yaw   = 0.0f;
    orientation->pitch = 0.0f;
    orientation->roll  = 0.0f;
}

float mpu6050_normalise_heading(float heading_deg)
{
    heading_deg = fmodf(heading_deg, 360.0f);
    if (heading_deg < 0.0f) heading_deg += 360.0f;
    return heading_deg;
}

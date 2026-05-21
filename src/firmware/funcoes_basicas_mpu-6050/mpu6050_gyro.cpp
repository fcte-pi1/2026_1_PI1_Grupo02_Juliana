#include "mpu6050_gyro.h"
#include <cstdio>

//  CONSTRUTOR — só guarda os parâmetros, não toca no hardware
MPU6050Gyro::MPU6050Gyro(i2c_inst_t* i2c_port, uint sda, uint scl)
    : i2c(i2c_port),
      sda_pin(sda),
      scl_pin(scl),
      gyro_z_offset(0.0f),
      yaw_degrees(0.0f) {}


//  writeRegister — escreve 1 byte em 1 registrador do MPU-6050
bool MPU6050Gyro::writeRegister(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = { reg, value };
    int bytes_written = i2c_write_blocking(
        i2c,
        MPU6050_I2C_ADDR,
        buffer,
        2,
        false               // enviar stop
    );
    return bytes_written == 2;
}

//  readRegisters — lê N bytes a partir de um registrador
bool MPU6050Gyro::readRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
    int result = i2c_write_blocking(i2c, MPU6050_I2C_ADDR, &reg, 1, true);
    if (result != 1) return false;

    // ler os bytes
    result = i2c_read_blocking(i2c, MPU6050_I2C_ADDR, buffer, length, false);
    return result == (int)length;
}

//  Os registradores 0x47 (Z_H) e 0x48 (Z_L) formam um int16_t.
int16_t MPU6050Gyro::readGyroZRaw() {
    uint8_t buf[2];
    if (!readRegisters(REG_GYRO_ZOUT_H, buf, 2)) return 0;
    return (int16_t)((buf[0] << 8) | buf[1]);
}


//  init — configura o I²C, acorda o sensor e ajusta os parâmetros
bool MPU6050Gyro::init() {
    i2c_init(i2c, 400 * 1000);  // Inicializa o periférico I²C do Pico em 400 kHz
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // Verifica se o chip responde lendo o WHO_AM_I (deve dar 0x68)
    uint8_t who = 0;
    if (!readRegisters(REG_WHO_AM_I, &who, 1)) return false;
    if (who != 0x68) {
        printf("WHO_AM_I errado: 0x%02X (esperado 0x68)\n", who);
        return false;
    }

    // PWR_MGMT_1: acorda o sensor (SLEEP=0) e usa PLL com X gyro
    if (!writeRegister(REG_PWR_MGMT_1, 0x01)) return false;
    sleep_ms(100);

    // CONFIG: DLPF ~42 Hz — filtra vibração sem grande atraso
    if (!writeRegister(REG_CONFIG, 0x03)) return false;

    // SMPLRT_DIV = 0 → Sample Rate = 1 kHz (com DLPF ativo)
    if (!writeRegister(REG_SMPLRT_DIV, 0x00)) return false;

    // GYRO_CONFIG: FS_SEL=0 → escala ±250°/s, sensibilidade 131 LSB/°/s
    if (!writeRegister(REG_GYRO_CONFIG, 0x00)) return false;

    last_update_time = get_absolute_time();
    return true;
}

//  calibrate — calcula o bias (offset) do giroscópio
//  IMPORTANTE: o robô precisa estar imóvel durante a calibração
void MPU6050Gyro::calibrate(uint num_samples) {
    printf("Calibrando giroscopio... NAO MEXA NO SENSOR!\n");

    float sum = 0.0f;
    for (uint i = 0; i < num_samples; i++) {
        sum += (float)readGyroZRaw();
        sleep_ms(2);
    }
    gyro_z_offset = sum / (float)num_samples;
    yaw_degrees = 0.0f;
    last_update_time = get_absolute_time();

    printf("Offset Z calculado: %.2f LSB\n", gyro_z_offset);
}

//  update — lê o giroscópio e integra para obter o ângulo
//  ângulo(t+Δt) = ângulo(t) + velocidade × Δt
void MPU6050Gyro::update() {
    absolute_time_t now = get_absolute_time();  // Δt em segundos desde a última leitura
    int64_t dt_us = absolute_time_diff_us(last_update_time, now);
    float dt = dt_us / 1000000.0f;
    last_update_time = now;

    // Leitura + remoção de bias
    int16_t raw = readGyroZRaw();
    float corrected = (float)raw - gyro_z_offset;

    // Conversão para °/s
    float gyro_z_dps = corrected / GYRO_SENSITIVITY_250DPS;

    // Zona morta: evita acúmulo de ruído quando parado
    if (gyro_z_dps > -0.2f && gyro_z_dps < 0.2f) {
        gyro_z_dps = 0.0f;
    }

    // Integração
    yaw_degrees += gyro_z_dps * dt;
}
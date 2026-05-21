#ifndef MPU6050_GYRO_H
#define MPU6050_GYRO_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

//  endereços dos registradores mpu-6050
//  Referência: datasheet "MPU-6000/MPU-6050 Register Map"

// Endereço I²C do chip (AD0 = GND => 0x68; AD0 = VCC => 0x69)
#define MPU6050_I2C_ADDR    0x68

// Registrador 25 (0x19) — divisor da taxa de amostragem
#define REG_SMPLRT_DIV      0x19

// Registrador 26 (0x1A) — configuração do filtro DLPF
#define REG_CONFIG          0x1A

// Registrador 27 (0x1B) — GYRO_CONFIG (escala do giroscópio)
#define REG_GYRO_CONFIG     0x1B

// Registrador 71 (0x47) — primeiro byte da leitura do eixo Z
#define REG_GYRO_ZOUT_H     0x47

// Registrador 107 (0x6B) — gerenciamento de energia
#define REG_PWR_MGMT_1      0x6B

// Registrador 117 (0x75) — identificação do chip
#define REG_WHO_AM_I        0x75

//  SENSIBILIDADE — depende da escala configurada
//  FS_SEL=0 → ±250 °/s  → 131   LSB por °/s
//  FS_SEL=1 → ±500 °/s  → 65.5
//  FS_SEL=2 → ±1000 °/s → 32.8
//  FS_SEL=3 → ±2000 °/s → 16.4
#define GYRO_SENSITIVITY_250DPS  131.0f


class MPU6050Gyro {
public:
    // Construtor: recebe a porta I²C e os pinos a usar
    MPU6050Gyro(i2c_inst_t* i2c_port, uint sda_pin, uint scl_pin);

    // Inicializa o I²C e configura o sensor.
    bool init();

    // Calcula o bias (offset) do giroscópio. Robô DEVE estar parado.
    void calibrate(uint num_samples = 1000);

    // Lê o giroscópio e atualiza o ângulo yaw acumulado.
    void update();

    // Retorna o ângulo acumulado no eixo Z, em graus.
    float getYaw() const { return yaw_degrees; }

    // Zera o ângulo (usar antes de cada giro de 90°).
    void resetYaw() { yaw_degrees = 0.0f; }

    // Exposto para os testes — leitura bruta do eixo Z
    int16_t getRawZ() { return readGyroZRaw(); }

private:
    // ---- atributos privados ----
    i2c_inst_t* i2c;
    uint sda_pin;
    uint scl_pin;

    float gyro_z_offset;
    float yaw_degrees;
    absolute_time_t last_update_time;

    // ---- métodos auxiliares ----
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t length);
    int16_t readGyroZRaw();
};

#endif
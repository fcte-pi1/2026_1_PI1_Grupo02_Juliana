#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdio.h>

#define VL53L0X_I2C_ADDR      0x29 
#define I2C_FREQ_HZ           400000 

#define PINO_SDA_S1           14
#define PINO_SCL_S1           15

#define PINO_SDA_S2           18  
#define PINO_SCL_S2           19  

// Registradores de teste
#define REG_IDENT_C0          0xC0  
#define REG_IDENT_C1          0xC1  
#define REG_IDENT_C2          0xC2  

/**
 * @brief  
 * Agora ela recebe 'i2c_inst_t *i2c_port' para saber qual caminho usar.
 */
bool vl53l0x_read_register(i2c_inst_t *i2c_port, uint8_t reg_index, uint8_t *data) {
    int resultado = i2c_write_blocking(i2c_port, VL53L0X_I2C_ADDR, &reg_index, 1, true);
    if (resultado != 1) return false;
    resultado = i2c_read_blocking(i2c_port, VL53L0X_I2C_ADDR, data, 1, false);
    return (resultado == 1);
}

/**
 * @brief 
 */
bool vl53l0x_validar_conexao(i2c_inst_t *i2c_port) {
    uint8_t val_c0 = 0, val_c1 = 0, val_c2 = 0;
    
    if (!vl53l0x_read_register(i2c_port, REG_IDENT_C0, &val_c0) ||
        !vl53l0x_read_register(i2c_port, REG_IDENT_C1, &val_c1) ||
        !vl53l0x_read_register(i2c_port, REG_IDENT_C2, &val_c2)) {
        return false;
    }
    
    return (val_c0 == 0xEE && val_c1 == 0xAA && val_c2 == 0x10);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("\n--- Inicializando Sensores (I2C Isolados) ---\n");

    i2c_init(i2c0, I2C_FREQ_HZ);
    gpio_set_function(PINO_SDA_S1, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL_S1, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA_S1);
    gpio_pull_up(PINO_SCL_S1);

    i2c_init(i2c1, I2C_FREQ_HZ);
    gpio_set_function(PINO_SDA_S2, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL_S2, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA_S2);
    gpio_pull_up(PINO_SCL_S2);

    bool s1_ok = vl53l0x_validar_conexao(i2c0);
    bool s2_ok = vl53l0x_validar_conexao(i2c1);

    while (true) {
        if (s1_ok) {
            printf("[Sensor 1 | i2c0]: Online!\n");
        } else {
            printf("[Sensor 1 | i2c0]: Falha na fiação.\n");
        }

        if (s2_ok) {
            printf("[Sensor 2 | i2c1]: Online!\n");
        } else {
            printf("[Sensor 2 | i2c1]: Falha na fiação.\n");
        }

        printf("---------------------------------------\n");
        sleep_ms(3000);
    }

    return 0;
}
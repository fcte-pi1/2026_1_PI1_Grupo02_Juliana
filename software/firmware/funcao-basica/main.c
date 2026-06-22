#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdio.h>

// Endereços I2C
#define VL53L0X_DEFAULT_ADDR   0x29 // endereço padrão: 0x57 deslocado um bit a direita
#define SENSOR1_NEW_ADDR       0x30 // novo endereço do sensor 1
#define SENSOR2_NEW_ADDR       0x31 // novo endereço do sensor 2

#define I2C_FREQ_HZ           400000 //define a velocidade de comunicação em 400kHz 
#define PINO_SDA              15 //GP15 para o SCL (conforme esquematico)
#define PINO_SCL              14 //GP14 para o SDA (conforme esquematico)

#define PINO_XSHUT_S1         2  // controla o reset do sensor 1
#define PINO_XSHUT_S2         3  // controla o reset do sensor 2

// essas tres linhas definem o endereço de identificação interno do chip:
#define REG_IDENT_C0          0xC0  
#define REG_IDENT_C1          0xC1  
#define REG_IDENT_C2          0xC2  

//define o registrador interno do sensor para mudar o endereço I2C
#define REG_I2C_SLAVE_DEVICE_ADDRESS   0x8A 

/**
 * @brief escreve 1 byte em um registrador de um dispositivo I2C específico
 */
//função de checagem de registrador, retorna 1 para true e 0 para false, recebe o valor do registrador e um ponteiro para salvar o valor lido
bool vl53l0x_write_register(uint8_t target_addr, uint8_t reg_index, uint8_t data) {
    uint8_t buffer[2] = {reg_index, data};
    int resultado = i2c_write_blocking(i2c0, target_addr, buffer, 2, false);
    return (resultado == 2);
}

/**
 * @brief Lê 1 byte de um registrador de um dispositivo I2C específico.
 */
bool vl53l0x_read_register(uint8_t target_addr, uint8_t reg_index, uint8_t *data) {
    int resultado = i2c_write_blocking(i2c0, target_addr, &reg_index, 1, true);
    if (resultado != 1) return false;
    resultado = i2c_read_blocking(i2c0, target_addr, data, 1, false);
    return (resultado == 1);
}

/**
 * @brief Altera o endereço I2C dinâmico do sensor.
 * @param current_addr Endereço atual do chip na linha (ex: 0x29)
 * @param new_addr Novo endereço desejado (ex: 0x30)
 */
bool vl53l0x_set_address(uint8_t current_addr, uint8_t new_addr) {
    // O chip armazena o endereço deslocado ou mascarado. Escrevemos os 7 bits puros.
    return vl53l0x_write_register(current_addr, REG_I2C_SLAVE_DEVICE_ADDRESS, new_addr & 0x7F);
}

/**
 * @brief Valida a conexão lendo os registradores de teste de um endereço específico.
 */
bool vl53l0x_validar_endereco(uint8_t addr) {
    uint8_t val_c0 = 0, val_c1 = 0, val_c2 = 0;
    if (!vl53l0x_read_register(addr, REG_IDENT_C0, &val_c0) ||
        !vl53l0x_read_register(addr, REG_IDENT_C1, &val_c1) ||
        !vl53l0x_read_register(addr, REG_IDENT_C2, &val_c2)) {
        return false;
    }
    return (val_c0 == 0xEE && val_c1 == 0xAA && val_c2 == 0x10);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("\n--- Inicializando Configuração Multissensor VL53L0X ---\n");
    
    // 1. Inicializa o barramento I2C básico
    i2c_init(i2c0, I2C_FREQ_HZ);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA);
    gpio_pull_up(PINO_SCL);

    // 2. Configura os pinos XSHUT como saídas digitais comuns
    gpio_init(PINO_XSHUT_S1);
    gpio_init(PINO_XSHUT_S2);
    gpio_set_dir(PINO_XSHUT_S1, GPIO_OUT);
    gpio_set_dir(PINO_XSHUT_S2, GPIO_OUT);

    // 3. Desliga AMBOS os sensores (Hardware Standby)
    gpio_put(PINO_XSHUT_S1, 0);
    gpio_put(PINO_XSHUT_S2, 0);
    sleep_ms(10); // Aguarda o reset físico dos chips

    // 4. Acorda apenas o SENSOR 1
    gpio_put(PINO_XSHUT_S1, 1);
    sleep_ms(5); // Aguarda o boot do firmware interno (tBOOT máx é 1.2ms)

    // 5. Muda o endereço do Sensor 1 de 0x29 para 0x30
    printf("Configurando Sensor 1...\n");
    if (!vl53l0x_set_address(VL53L0X_DEFAULT_ADDR, SENSOR1_NEW_ADDR)) {
        printf("Erro: Não foi possível alterar o endereço do Sensor 1.\n");
    }

    // 6. Acorda o SENSOR 2 (Ele vai iniciar no endereço padrão 0x29)
    gpio_put(PINO_XSHUT_S2, 1);
    sleep_ms(5);

    // 7. Opcional: Muda o endereço do Sensor 2 de 0x29 para 0x31 (para liberar o 0x29)
    printf("Configurando Sensor 2...\n");
    if (!vl53l0x_set_address(VL53L0X_DEFAULT_ADDR, SENSOR2_NEW_ADDR)) {
        printf("Erro: Não foi possível alterar o endereço do Sensor 2.\n");
    }

    // 8. Teste de validação final com os novos endereços individuais
    bool s1_ok = vl53l0x_validar_endereco(SENSOR1_NEW_ADDR);
    bool s2_ok = vl53l0x_validar_endereco(SENSOR2_NEW_ADDR);

    while (true) {
        if (s1_ok) {
            printf("[Sensor 1 - Disp 0x%02X]: Online!\n", SENSOR1_NEW_ADDR);
        } else {
            printf("[Sensor 1]: Falha de comunicação.\n");
        }

        if (s2_ok) {
            printf("[Sensor 2 - Disp 0x%02X]: Online!\n", SENSOR2_NEW_ADDR);
        } else {
            printf("[Sensor 2]: Falha de comunicação.\n");
        }

        printf("---------------------------------------\n");
        sleep_ms(3000);
    }

    return 0;
}

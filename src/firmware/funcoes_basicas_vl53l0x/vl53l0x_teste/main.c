#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x.h"

// --- Pinos I2C ---
#define I2C0_SDA_PIN 4
#define I2C0_SCL_PIN 5

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

// IMPORTANTE: os pull-ups internos do RP2040 (~50k) são fracos demais para
// I2C confiável a 400kHz com fios de breakout. Use resistores EXTERNOS de
// 4.7k em SDA/SCL. Se mesmo assim houver instabilidade, baixe para 100*1000.
#define I2C_BAUDRATE (400 * 1000)

static VL53L0X sensor_esq; // i2c0
static VL53L0X sensor_dir; // i2c1

static void setup_i2c_bus(i2c_inst_t *i2c, uint sda, uint scl) {
    i2c_init(i2c, I2C_BAUDRATE);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

int main() {
    stdio_init_all();
    sleep_ms(1500); // tempo para o USB enumerar

    printf("\n=========================================\n");
    printf(" VISAO ESTEREO - DRIVER VL53L0X COMPLETO \n");
    printf("=========================================\n");

    setup_i2c_bus(i2c0, I2C0_SDA_PIN, I2C0_SCL_PIN);
    setup_i2c_bus(i2c1, I2C1_SDA_PIN, I2C1_SCL_PIN);

    printf("A inicializar os sensores...\n");

    bool ok_esq = vl53l0x_init(&sensor_esq, i2c0, true);
    bool ok_dir = vl53l0x_init(&sensor_dir, i2c1, true);

    printf("Sensor ESQ (i2c0): %s\n", ok_esq ? "OK" : "FALHOU");
    printf("Sensor DIR (i2c1): %s\n", ok_dir ? "OK" : "FALHOU");

    if (!ok_esq || !ok_dir) {
        printf("\n[ERRO] Pelo menos um sensor nao inicializou.\n");
        printf("Verifique: alimentacao 3.3V, pull-ups externos 4.7k,\n");
        printf("ligacoes SDA/SCL e se o pino XSHUT esta em nivel alto.\n");
        // Continua mesmo assim para tentar ler o que respondeu.
    }

    // Opcional: timing budget maior = leituras mais precisas (e mais lentas).
    // Default da init = 33ms. Exemplo para 50ms:
    // vl53l0x_set_measurement_timing_budget(&sensor_esq, 50000);
    // vl53l0x_set_measurement_timing_budget(&sensor_dir, 50000);

    printf("\nLeituras iniciadas! Pode aproximar as maos.\n\n");

    while (true) {
        uint16_t dist_esq = ok_esq ? vl53l0x_read_range_single_millimeters(&sensor_esq) : 0;
        uint16_t dist_dir = ok_dir ? vl53l0x_read_range_single_millimeters(&sensor_dir) : 0;

        printf("Esq: %5u mm  |  Dir: %5u mm\n", dist_esq, dist_dir);

        sleep_ms(50);
    }

    return 0;
}

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x.h"

// --- Pinos I2C ---
#define I2C0_SDA_PIN 4
#define I2C0_SCL_PIN 5

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

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
    sleep_ms(100); // estabilização mínima de hardware

    setup_i2c_bus(i2c0, I2C0_SDA_PIN, I2C0_SCL_PIN);
    setup_i2c_bus(i2c1, I2C1_SDA_PIN, I2C1_SCL_PIN);

    bool ok_esq = vl53l0x_init(&sensor_esq, i2c0, true);
    bool ok_dir = vl53l0x_init(&sensor_dir, i2c1, true);

    // Timing budget opcional: maior = mais preciso, mais lento.
    // Comente se quiser usar o default (33ms).
    // vl53l0x_set_measurement_timing_budget(&sensor_esq, 50000);
    // vl53l0x_set_measurement_timing_budget(&sensor_dir, 50000);

    while (true) {
        uint16_t dist_esq = ok_esq ? vl53l0x_read_range_single_millimeters(&sensor_esq) : 0;
        uint16_t dist_dir = ok_dir ? vl53l0x_read_range_single_millimeters(&sensor_dir) : 0;

        // Use dist_esq e dist_dir aqui para a sua lógica (motores, navegação, etc.)
        // Exemplo: if (dist_esq < 100) { /* virar à direita */ }

        sleep_ms(50);
    }

    return 0;
}

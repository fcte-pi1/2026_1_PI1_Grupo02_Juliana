#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x.h"

int main() {
    stdio_init_all();
    sleep_ms(2000); 

    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    VL53L0X sensor_esquerdo;
    VL53L0X sensor_direito;

    vl53l0x_init(&sensor_esquerdo, i2c0);
    vl53l0x_init(&sensor_direito, i2c1);

    printf("Sensores prontos e calibrados!\n");

    while (true) {
        uint16_t dist_esq = vl53l0x_read_range_single_millimeters(&sensor_esquerdo);
        uint16_t dist_dir = vl53l0x_read_range_single_millimeters(&sensor_direito);

        printf("Esq: %d mm \t Dir: %d mm\n", dist_esq, dist_dir);
        sleep_ms(50);
    }

    return 0;
}
// Scan I2C em TODOS os pares de pinos validos do RP2040, pra descobrir em quais
// pinos os sensores estao ligados (ou confirmar que nenhum responde = problema
// de alimentacao/GND, nao de pinagem).
//
// NAO aciona motores nem nada que se mexa - seguro pra rodar sem supervisao.
// Le apenas I2C, temporariamente, em cada par; libera os pinos entre um e outro.

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

struct Par { i2c_inst_t *bus; const char *nome; uint sda; uint scl; };

// Pares SDA/SCL validos no RP2040 (i2c0 e i2c1 alternam por grupo de 4 pinos).
static Par pares[] = {
    {i2c0, "i2c0", 0, 1},  {i2c0, "i2c0", 4, 5},  {i2c0, "i2c0", 8, 9},
    {i2c0, "i2c0", 12, 13},{i2c0, "i2c0", 16, 17},{i2c0, "i2c0", 20, 21},
    {i2c1, "i2c1", 2, 3},  {i2c1, "i2c1", 6, 7},  {i2c1, "i2c1", 10, 11},
    {i2c1, "i2c1", 14, 15},{i2c1, "i2c1", 18, 19},{i2c1, "i2c1", 26, 27},
};

static int scan_par(const Par &p) {
    i2c_init(p.bus, 100 * 1000);
    gpio_set_function(p.sda, GPIO_FUNC_I2C);
    gpio_set_function(p.scl, GPIO_FUNC_I2C);
    gpio_pull_up(p.sda);
    gpio_pull_up(p.scl);
    sleep_ms(5);

    int n = 0;
    uint8_t rx;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_read_timeout_us(p.bus, addr, &rx, 1, false, 1500) >= 0) {
            printf(" 0x%02X", addr);
            n++;
        }
    }

    // Libera os pinos e o barramento pro proximo par.
    gpio_set_function(p.sda, GPIO_FUNC_SIO);
    gpio_set_function(p.scl, GPIO_FUNC_SIO);
    gpio_disable_pulls(p.sda);
    gpio_disable_pulls(p.scl);
    i2c_deinit(p.bus);
    return n;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    printf("\r\n=== SCAN I2C EM TODOS OS PARES DE PINOS ===\r\n");
    printf("(procurando 0x68 = MPU6050, 0x29 = VL53L0X)\r\n\r\n");

    while (true) {
        int total = 0;
        for (const Par &p : pares) {
            printf("%s  SDA=GP%-2u SCL=GP%-2u:", p.nome, p.sda, p.scl);
            int n = scan_par(p);
            if (n == 0) printf(" (nada)");
            printf("\r\n");
            total += n;
        }
        printf(">>> total de dispositivos: %d", total);
        if (total == 0)
            printf("  -> nenhum I2C em pino nenhum. E alimentacao/GND, nao pinagem.");
        printf("\r\n\r\n");
        sleep_ms(4000);
    }
}

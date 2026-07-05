// Teste de hardware do micromouse (Pico W) - PINAGEM REAL da montagem (folha do
// Samuel, eletronica, 2026-07-04). Diverge da pinagem antiga do firmware.
//
//   I2C0 (GP0 SDA / GP1 SCL): MPU6050 (0x68) + VL53L0X DIREITA (0x29)
//   I2C1 (GP2 SDA / GP3 SCL): VL53L0X ESQUERDA (0x29)
//   HC-SR04: TRIG GP6 / ECHO GP7
//   Ponte H TB6612FNG:
//     STBY = GP22 (precisa HIGH pra sair do standby)
//     Motor DIREITA (A): PWMA=GP20, AIN1=GP17, AIN2=GP16
//     Motor ESQUERDA (B): PWMB=GP21, BIN1=GP18, BIN2=GP19
//   Energia: tensao GP26 (ADC0), corrente GP27 (ADC1)  [a confirmar]
//
// Os motores so giram com a bateria ligada (VM da ponte H). Os sensores I2C
// rodam no 3V3 da Pico (USB), entao respondem sem bateria.

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "Motor.h"
#include "EnergyMonitor.h"

// Pinos da ponte H conforme a folha do Samuel.
static const uint STBY_PIN = 22;
static Motor motorDir(20, 17, 16); // PWMA, AIN1, AIN2
static Motor motorEsq(21, 18, 19); // PWMB, BIN1, BIN2
static EnergyMonitor energia;

static uint32_t agora_ms() { return to_ms_since_boot(get_absolute_time()); }
static void led(bool on) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on ? 1 : 0); }

static void i2c_setup(i2c_inst_t *bus, uint sda, uint scl) {
    i2c_init(bus, 400 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

static int i2c_scan(const char *nome, i2c_inst_t *bus) {
    printf("  Scan %s:", nome);
    int achados = 0;
    uint8_t rx;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_read_timeout_us(bus, addr, &rx, 1, false, 1000) >= 0) {
            printf(" 0x%02X", addr);
            achados++;
        }
    }
    if (achados == 0) printf(" (nada)");
    printf("\r\n");
    return achados;
}

static void testar_motor(const char *nome, Motor &m) {
    printf("  %s: frente 40%%...\r\n", nome);
    m.setVelocidade(100); sleep_ms(1200); m.setVelocidade(0); sleep_ms(400);
    printf("  %s: re 40%%...\r\n", nome);
    m.setVelocidade(-100); sleep_ms(1200); m.parar(); sleep_ms(400);
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    printf("\r\n============================================\r\n");
    printf(" TESTE DE HARDWARE - MICROMOUSE (pinagem real)\r\n");
    printf("============================================\r\n");

    bool led_ok = (cyw43_arch_init() == 0);
    printf("[%s] CYW43 / LED onboard\r\n", led_ok ? "OK" : "--");

    // STBY do TB6612 em HIGH: sem isso o driver fica em standby (nenhum motor gira).
    gpio_init(STBY_PIN);
    gpio_set_dir(STBY_PIN, GPIO_OUT);
    gpio_put(STBY_PIN, 1);
    printf("[OK] STBY (GP22) em HIGH -> driver habilitado\r\n");

    // --- Sensores I2C primeiro (rodam sem bateria) ---
    printf("Scan I2C (esperado: 0x68 MPU + 0x29 VL53 no i2c0; 0x29 VL53 no i2c1):\r\n");
    i2c_setup(i2c0, 0, 1);
    i2c_setup(i2c1, 2, 3);
    int n0 = i2c_scan("i2c0 (GP0/GP1)", i2c0);
    int n1 = i2c_scan("i2c1 (GP2/GP3)", i2c1);
    printf("Resumo I2C: i2c0=%d, i2c1=%d\r\n", n0, n1);

    // --- Motores (rodas no ar; so giram com bateria na ponte H) ---
    motorDir.inicializar();
    motorEsq.inicializar();
    printf("Testando motores (rodas no ar; precisa bateria na ponte H):\r\n");
    testar_motor("Motor DIR", motorDir);
    testar_motor("Motor ESQ", motorEsq);
    printf("  Ambos frente 50%%...\r\n");
    motorDir.setVelocidade(128); motorEsq.setVelocidade(128); sleep_ms(1500);
    motorDir.parar(); motorEsq.parar();
    printf("  Motores: fim.\r\n");

    // --- Energia ---
    energia.inicializar();
    printf("[OK] Energia inicializada.\r\n\r\n");

    printf("Leitura continua:\r\n\r\n");
    while (true) {
        uint32_t t = agora_ms();
        energia.atualizar(t);
        printf("[ENERGIA] V=%.2f I=%.2fA bat=%d%%\r\n",
               energia.tensao_v(), energia.corrente_a(), energia.bateria_pct());
        if (led_ok) led((t / 500) % 2);
        sleep_ms(500);
    }
}

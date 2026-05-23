#include <stdio.h>
#include "pico/stdlib.h"
#include "mpu6050_gyro.h"

// Pinos do MPU-6050 (I2C0)
#define MPU_SDA  4
#define MPU_SCL  5

MPU6050Gyro imu(i2c0, MPU_SDA, MPU_SCL);

int main() {
    stdio_init_all();
    sleep_ms(3000);  // tempo para conectar ao terminal serial

    printf("=== Modulo Giroscopio MPU-6050 ===\n\n");

    if (!imu.init()) {
        printf("Falha ao iniciar o MPU-6050!\n");
        while (true) tight_loop_contents();
    }
    printf("MPU-6050 inicializado.\n");

    sleep_ms(1000);
    imu.calibrate(1000);
    printf("Pronto. Imprimindo yaw a cada 100 ms.\n\n");

    // Loop principal: atualiza rapidamente, imprime devagar
    absolute_time_t last_print = get_absolute_time();
    while (true) {
        imu.update();  // chama o mais rápido possível

        // Imprime a cada 100 ms (sem segurar o loop com sleep grande)
        if (absolute_time_diff_us(last_print, get_absolute_time()) > 100000) {
            printf("Yaw: %.2f°\n", imu.getYaw());
            last_print = get_absolute_time();
        }

        sleep_ms(2);  // ~500 Hz no loop de leitura
    }
}
#include <stdio.h>
#include "pico/stdlib.h"
#include "motor_n20.h"
#include "encoder.h"

// Reducao da caixa do motor usado (coluna "Ratio" do datasheet JGA12-N10B:
// 10, 30, 50, 100, 150, 210, 298 ou 380). Ajuste para o seu motor.
#define GEAR_RATIO 100

int main(void) {
    stdio_init_all();

    motor_n20_t motor;
    encoder_t encoder;

    // Ponte H (TB6612FNG): PWM=GP15, IN1=GP14, IN2=GP13
    motor_n20_init(&motor, 15, 14, 13);

    // Encoder: canal A (C1/verde)=GP16, canal B (C2/amarelo)=GP17
    encoder_init(&encoder, 16, 17, GEAR_RATIO);

    while (true) {
        // Acelera para frente
        motor_n20_set_speed(&motor, 180);
        for (int i = 0; i < 20; i++) {
            sleep_ms(100);
            printf("FRENTE | count=%ld  voltas=%.2f  rpm=%.1f\n",
                   encoder_get_count(&encoder),
                   encoder_get_revolutions(&encoder),
                   encoder_get_rpm(&encoder));
        }

        // Para por inercia
        motor_n20_stop(&motor);
        sleep_ms(1000);

        // Acelera para tras
        motor_n20_set_speed(&motor, -180);
        for (int i = 0; i < 20; i++) {
            sleep_ms(100);
            printf("RE     | count=%ld  voltas=%.2f  rpm=%.1f\n",
                   encoder_get_count(&encoder),
                   encoder_get_revolutions(&encoder),
                   encoder_get_rpm(&encoder));
        }

        // Frenagem ativa
        motor_n20_brake(&motor);
        sleep_ms(1000);
    }
}

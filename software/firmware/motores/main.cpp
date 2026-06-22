#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "Motor.h"
#include "PID.h"
#include "WatchdogMotor.h"

// Motor Esquerdo: PWM GP15, DIR1 GP14, DIR2 GP13
// Motor Direito:  PWM GP12, DIR1 GP11, DIR2 GP10
Motor motorEsq(15, 14, 13);
Motor motorDir(12, 11, 10);
WatchdogMotor watchdog(&motorEsq, &motorDir, 2000);

static inline uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    cyw43_arch_init();

    printf("\r\n=== Teste de Motores ===\r\n");
    printf("Pinos: ESQ(PWM=GP15 D1=GP14 D2=GP13) DIR(PWM=GP12 D1=GP11 D2=GP10)\r\n\r\n");

    motorEsq.inicializar();
    motorDir.inicializar();

    while (true) {
        watchdog.alimentar();

        printf("[1/5] Frente - velocidade 150\r\n");
        motorEsq.setVelocidade(150);
        motorDir.setVelocidade(150);
        sleep_ms(2000);
        watchdog.alimentar();

        printf("[2/5] Parar\r\n");
        motorEsq.parar();
        motorDir.parar();
        sleep_ms(1000);
        watchdog.alimentar();

        printf("[3/5] Re - velocidade -150\r\n");
        motorEsq.setVelocidade(-150);
        motorDir.setVelocidade(-150);
        sleep_ms(2000);
        watchdog.alimentar();

        printf("[4/5] Curva direita\r\n");
        motorEsq.setVelocidade(150);
        motorDir.setVelocidade(50);
        sleep_ms(1500);
        watchdog.alimentar();

        printf("[5/5] Curva esquerda\r\n");
        motorEsq.setVelocidade(50);
        motorDir.setVelocidade(150);
        sleep_ms(1500);
        watchdog.alimentar();

        printf("[CICLO] Reiniciando sequencia...\r\n\r\n");
        motorEsq.parar();
        motorDir.parar();
        sleep_ms(1000);
    }

    return 0;
}

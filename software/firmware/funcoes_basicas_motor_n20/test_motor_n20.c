#include <stdio.h>
#include "pico/stdlib.h"
#include "motor_n20.h"
#include "encoder.h"

#define GEAR_RATIO 100

// Firmware de validacao do conjunto motor N20 + encoder em quadratura.
// Verifica: sentido de rotacao, contagem do encoder, calculo de RPM e a
// coerencia entre o sentido comandado e o sinal da contagem.
int main(void) {
    stdio_init_all();
    sleep_ms(2000); // aguarda o monitor serial conectar

    motor_n20_t motor;
    encoder_t encoder;
    motor_n20_init(&motor, 15, 14, 13);
    encoder_init(&encoder, 16, 17, GEAR_RATIO);

    printf("=== TESTE/VALIDACAO - Motor N20 (JGA12-N10B) + Encoder ===\n");
    printf("Reducao configurada: %d:1  ->  %.0f contagens por volta do eixo\n\n",
           GEAR_RATIO, (float)ENCODER_PPR_MOTOR * ENCODER_QUAD_MULT * GEAR_RATIO);

    // --- Teste 1: motor parado, encoder nao deve contar sozinho ---
    printf("[1] Motor parado por 2s - encoder deve permanecer estavel...\n");
    encoder_reset(&encoder);
    sleep_ms(2000);
    int32_t parado = encoder_get_count(&encoder);
    printf("    count em repouso: %ld %s\n\n", parado,
           (parado == 0) ? "(OK)" : "(AVISO: ruido/IRQ espuria?)");

    // --- Teste 2: frente -> contagem deve aumentar ---
    printf("[2] FRENTE (speed=+150) por 2s...\n");
    encoder_reset(&encoder);
    motor_n20_set_speed(&motor, 150);
    sleep_ms(2000);
    motor_n20_stop(&motor);
    int32_t frente = encoder_get_count(&encoder);
    float rpm_frente = encoder_get_rpm(&encoder);
    printf("    count=%ld  rpm(ultimo intervalo)=%.1f\n", frente, rpm_frente);
    if (frente == 0)      printf("    [AVISO] count=0 -> verifique alimentacao do encoder (VCC/GND) e fios C1/C2\n");
    else if (frente < 0)  printf("    [AVISO] count negativo na frente -> troque os pinos A/B (ou IN1/IN2)\n");
    else                  printf("    (OK) contagem positiva no sentido frente\n");
    printf("\n");

    sleep_ms(1000);

    // --- Teste 3: re -> contagem deve diminuir ---
    printf("[3] RE (speed=-150) por 2s...\n");
    encoder_reset(&encoder);
    motor_n20_set_speed(&motor, -150);
    sleep_ms(2000);
    motor_n20_stop(&motor);
    int32_t re = encoder_get_count(&encoder);
    printf("    count=%ld\n", re);
    if (re >= 0) printf("    [AVISO] esperado count negativo na re -> verifique fiacao\n");
    else         printf("    (OK) contagem negativa no sentido re\n");
    printf("\n");

    // --- Monitor continuo ---
    printf("[4] Monitor continuo (frente a meia velocidade)...\n");
    encoder_reset(&encoder);
    motor_n20_set_speed(&motor, 128);
    while (true) {
        sleep_ms(200);
        printf("count=%6ld  voltas=%7.2f  rpm=%6.1f\n",
               encoder_get_count(&encoder),
               encoder_get_revolutions(&encoder),
               encoder_get_rpm(&encoder));
    }
}

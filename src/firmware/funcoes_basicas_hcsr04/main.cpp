#include "pico/stdlib.h"
#include "hcsr04.h"
#include <stdio.h>

// Intervalo entre medidas — respeita o ciclo mínimo de 60 ms do datasheet
#define LOOP_MS 100

// Distância (cm) abaixo da qual consideramos uma parede próxima ao robô
#define WALL_THRESHOLD_CM 10.0f

int main(void)
{
    stdio_init_all();
    sleep_ms(2000); // esperando a conexão serial estabilizar

    hcsr04_init();

    while (true) {
        float distancia = hcsr04_medir_filtrado_cm(5);

        if (distancia == HCSR04_INVALID_READING) {
            printf("Distancia: --- (timeout ou fora de faixa)\n");
        } else {
            const char *status = (distancia < WALL_THRESHOLD_CM) ? "PAREDE" : "LIVRE";
            printf("Distancia: %6.2f cm  | %s\n", distancia, status);
        }

        sleep_ms(LOOP_MS);
    }

    return 0;
}

// Demo / validacao do DistanceSensors (HU04 - medicao de distancia).
// Roda no Pico W: le os 3 sensores (2x VL53L0X + HC-SR04) a cada 50 ms e
// imprime a distancia por direcao e o alerta de colisao iminente no serial.
#include <stdio.h>
#include "pico/stdlib.h"
#include "DistanceSensors.h"

static const char* nome_dir(DirecaoDist d) {
    switch (d) {
        case DIR_FRENTE:   return "FRENTE";
        case DIR_ESQUERDA: return "ESQ";
        case DIR_DIREITA:  return "DIR";
        default:           return "-";
    }
}

static void imprime_dist(DistanceSensors& s, DirecaoDist d) {
    float cm = s.distancia_cm(d);
    if (cm < 0) printf("%s: --   ", nome_dir(d));
    else        printf("%s: %5.1fcm ", nome_dir(d), cm);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("\r\n=== Demo DistanceSensors (HU04) ===\r\n");
    printf("Amostragem %d ms | colisao < %.1f cm\r\n\r\n",
           DIST_INTERVALO_MS, (float)DIST_COLISAO_CM);

    DistanceSensors sensores;
    sensores.inicializar();

    while (true) {
        uint32_t agora = to_ms_since_boot(get_absolute_time());
        sensores.atualizar(agora);

        imprime_dist(sensores, DIR_FRENTE);
        imprime_dist(sensores, DIR_ESQUERDA);
        imprime_dist(sensores, DIR_DIREITA);

        if (sensores.colisao_iminente())
            printf("| COLISAO IMINENTE (%s)", nome_dir(sensores.direcao_colisao()));
        printf("\r\n");

        sleep_ms(DIST_INTERVALO_MS);
    }
}

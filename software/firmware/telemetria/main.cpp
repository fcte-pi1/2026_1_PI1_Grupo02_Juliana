#include <stdio.h>
#include "pico/stdlib.h"
#include "MazeMapper.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("\r\n=== Teste de Telemetria / MazeMapper ===\r\n\r\n");

    const char* run_id = "micromouse_001";
    char payload[512];

    // Simula exploração de algumas células
    atualizar_mapa(0, 0, false, true,  true,  false); // célula (0,0): parede ao norte e oeste
    atualizar_mapa(1, 0, false, true,  true,  false);
    atualizar_mapa(1, 1, true,  false, false, true);
    atualizar_mapa(2, 1, false, true,  false, true);

    int pos_x = 1, pos_y = 1;

    while (true) {
        gerar_payload_telemetria(payload, sizeof(payload), pos_x, pos_y, run_id);

        printf("[TELEMETRIA] x=%d y=%d\r\n%s\r\n\r\n", pos_x, pos_y, payload);
        printf("Explorado: %.1f%%\r\n\r\n", calcular_porcentagem_explorada());

        sleep_ms(2000);
    }

    return 0;
}

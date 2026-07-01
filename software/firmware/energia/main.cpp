// Demo / validacao do EnergyMonitor (HU21 - monitoramento de tensao).
// Roda no Pico W: le a tensao da fonte a cada 500 ms, imprime em volts e
// dispara os alertas de tensao baixa e oscilacao no terminal serial.
#include <stdio.h>
#include "pico/stdlib.h"
#include "EnergyMonitor.h"

static const char* nome_evento(EventoEnergia e) {
    switch (e) {
        case EVT_TENSAO_BAIXA:     return "TENSAO_BAIXA";
        case EVT_TENSAO_OSCILACAO: return "TENSAO_OSCILACAO";
        default:                   return "NENHUM";
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("\r\n=== Demo EnergyMonitor (HU21 - tensao) ===\r\n");
    printf("Amostragem a cada %d ms | min operacional %.2f V\r\n\r\n",
           ENERGIA_AMOSTRAGEM_MS, (float)ENERGIA_TENSAO_MIN_OP_V);

    EnergyMonitor energia;
    energia.inicializar();

    while (true) {
        uint32_t agora = to_ms_since_boot(get_absolute_time());
        energia.atualizar(agora);

        EventoEnergia evt = energia.consumir_evento();
        if (evt != EVT_NENHUM) {
            // Timestamp aproximado (ms desde boot) no log de oscilacao/alerta.
            printf("[ALERTA @%lums] %s: %s\r\n",
                   agora, nome_evento(evt), energia.ultimo_detalhe());
        } else {
            printf("Tensao: %.2f V\r\n", energia.tensao_v());
        }

        sleep_ms(ENERGIA_AMOSTRAGEM_MS);
    }
}

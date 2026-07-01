// Demo / validacao do EnergyMonitor.
// HU21 (tensao) + HU16 (bateria): roda no Pico W, le a fonte a cada 500 ms,
// imprime tensao e nivel de bateria e dispara os alertas no terminal serial.
#include <stdio.h>
#include "pico/stdlib.h"
#include "EnergyMonitor.h"

static const char* nome_evento(EventoEnergia e) {
    switch (e) {
        case EVT_TENSAO_BAIXA:     return "TENSAO_BAIXA";
        case EVT_TENSAO_OSCILACAO: return "TENSAO_OSCILACAO";
        case EVT_BATERIA_BAIXA:    return "BATERIA_BAIXA";
        case EVT_BATERIA_CRITICA:  return "BATERIA_CRITICA";
        default:                   return "NENHUM";
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("\r\n=== Demo EnergyMonitor (HU21 tensao + HU16 bateria) ===\r\n");
    printf("Amostragem %d ms | min op %.2f V | alertas bat %d%%/%d%%\r\n\r\n",
           ENERGIA_AMOSTRAGEM_MS, (float)ENERGIA_TENSAO_MIN_OP_V,
           ENERGIA_BAT_ALERTA_PCT, ENERGIA_BAT_CRITICO_PCT);

    EnergyMonitor energia;
    energia.inicializar();

    while (true) {
        uint32_t agora = to_ms_since_boot(get_absolute_time());
        energia.atualizar(agora);

        printf("Tensao: %.2f V | Bateria: %d%%\r\n",
               energia.tensao_v(), energia.bateria_pct());

        // Drena todos os alertas gerados neste ciclo.
        EventoEnergia evt;
        while ((evt = energia.consumir_evento()) != EVT_NENHUM) {
            printf("  [ALERTA @%lums] %s: %s\r\n",
                   agora, nome_evento(evt), energia.ultimo_detalhe());
        }

        sleep_ms(ENERGIA_AMOSTRAGEM_MS);
    }
}

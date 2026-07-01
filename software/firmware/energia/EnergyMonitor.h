#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H

#include "pico/stdlib.h"
#include "config.h"

// Os drivers de sensor sao C puro: linkagem C ao incluir daqui (C++).
extern "C" {
#include "voltage_sensor.h"
}

// Eventos de energia edge-triggered (disparam uma vez na transicao), pra o
// firmware principal publicar como evento MQTT / gravar no log de operacao.
enum EventoEnergia {
    EVT_NENHUM = 0,
    EVT_TENSAO_BAIXA,       // HU21: tensao abaixo do minimo operacional
    EVT_TENSAO_OSCILACAO,   // HU21: variacao brusca de tensao
};

// Monitor de energia do micromouse.
// HU21: monitora a tensao da fonte, exibe em volts, alerta em queda e
//       registra oscilacoes. As HUs 16 (bateria) e 10 (consumo) sao
//       adicionadas nas etapas seguintes sobre este mesmo modulo.
class EnergyMonitor {
public:
    EnergyMonitor();

    // Inicializa o ADC e o sensor de tensao. Chamar uma vez no boot.
    void inicializar();

    // Le os sensores respeitando ENERGIA_AMOSTRAGEM_MS (nao bloqueia entre
    // amostras). Deve ser chamada a cada iteracao do loop principal.
    void atualizar(uint32_t agora_ms);

    // Ultima tensao lida, em volts.
    float tensao_v() const { return _tensao_v; }

    // Retorna e consome o evento pendente (edge-triggered). EVT_NENHUM se nada.
    // Apos um evento != EVT_NENHUM, ultimo_detalhe() traz o texto com o valor.
    EventoEnergia consumir_evento();
    const char* ultimo_detalhe() const { return _detalhe; }

private:
    voltage_sensor_t _vsensor;
    uint32_t _ultima_amostra_ms;
    float    _tensao_v;
    float    _tensao_ant_v;        // amostra anterior (deteccao de oscilacao)
    bool     _tensao_baixa_ativa;  // estado com histerese (evita alerta piscando)
    EventoEnergia _evento_pendente;
    char     _detalhe[64];

    void _avaliar_tensao();
};

#endif // ENERGY_MONITOR_H

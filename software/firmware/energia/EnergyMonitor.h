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
    EVT_BATERIA_BAIXA,      // HU16: bateria <= 20%
    EVT_BATERIA_CRITICA,    // HU16: bateria <= 5%
};

// Monitor de energia do micromouse.
// HU21: monitora a tensao da fonte (volts, alerta de queda, log de oscilacao).
// HU16: converte a tensao em nivel percentual de bateria e alerta em 20% e 5%.
// HU10 (consumo) e adicionada na etapa seguinte sobre este mesmo modulo.
class EnergyMonitor {
public:
    EnergyMonitor();

    // Inicializa o ADC e o sensor de tensao. Chamar uma vez no boot.
    void inicializar();

    // Le os sensores respeitando ENERGIA_AMOSTRAGEM_MS (nao bloqueia entre
    // amostras). Deve ser chamada a cada iteracao do loop principal.
    void atualizar(uint32_t agora_ms);

    float tensao_v()    const { return _tensao_v; }     // ultima tensao lida (V)
    int   bateria_pct() const { return _bateria_pct; }  // nivel de bateria (0-100)

    // Desenfileira o proximo evento pendente (edge-triggered). EVT_NENHUM quando
    // a fila esvazia. Apos um evento != EVT_NENHUM, ultimo_detalhe() traz o texto
    // com o valor medido. Chamar em laco ate EVT_NENHUM pra drenar todos.
    EventoEnergia consumir_evento();
    const char* ultimo_detalhe() const { return _detalhe; }

    // Converte uma tensao de bateria (V) em nivel percentual (0-100), clampeado.
    // Estatico: util pra teste e reuso sem instancia.
    static int tensao_para_pct(float v);

private:
    voltage_sensor_t _vsensor;
    uint32_t _ultima_amostra_ms;
    float    _tensao_v;
    float    _tensao_ant_v;        // amostra anterior (deteccao de oscilacao)
    int      _bateria_pct;
    bool     _tensao_baixa_ativa;  // estados com histerese (evitam alerta piscando)
    bool     _bat_baixa_ativa;
    bool     _bat_critica_ativa;

    // Fila circular de eventos: um ciclo pode gerar mais de um alerta
    // (ex. tensao baixa + bateria critica juntas).
    static const int EVT_FILA_MAX = 8;
    struct EventoLog { EventoEnergia tipo; char detalhe[64]; };
    EventoLog _fila[EVT_FILA_MAX];
    int  _fila_ini;
    int  _fila_fim;
    char _detalhe[64];

    void _enfileirar(EventoEnergia tipo, const char* detalhe);
    void _avaliar_tensao();
    void _avaliar_bateria();
};

#endif // ENERGY_MONITOR_H

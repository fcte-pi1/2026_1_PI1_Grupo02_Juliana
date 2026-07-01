#include "EnergyMonitor.h"
#include <stdio.h>
#include <math.h>

EnergyMonitor::EnergyMonitor()
    : _ultima_amostra_ms(0),
      _tensao_v(0.0f),
      _tensao_ant_v(0.0f),
      _tensao_baixa_ativa(false),
      _evento_pendente(EVT_NENHUM) {
    _detalhe[0] = '\0';
}

void EnergyMonitor::inicializar() {
    voltage_sensor_init(&_vsensor, ENERGIA_TENSAO_GPIO, ENERGIA_TENSAO_DIVIDER_RATIO);
    // Primeira leitura pra semear o valor anterior (evita falso "oscilacao" no boot).
    _tensao_v     = voltage_sensor_read_v(&_vsensor);
    _tensao_ant_v = _tensao_v;
}

void EnergyMonitor::atualizar(uint32_t agora_ms) {
    if (agora_ms - _ultima_amostra_ms < ENERGIA_AMOSTRAGEM_MS)
        return;
    _ultima_amostra_ms = agora_ms;

    _tensao_ant_v = _tensao_v;
    _tensao_v     = voltage_sensor_read_v(&_vsensor);

    _avaliar_tensao();
}

// HU21 - avalia queda de tensao (com histerese) e oscilacao brusca.
void EnergyMonitor::_avaliar_tensao() {
    // Alerta de tensao baixa: liga ao cruzar o minimo pra baixo, so desliga
    // depois de subir o minimo + histerese (evita piscar em torno do limiar).
    if (!_tensao_baixa_ativa && _tensao_v < ENERGIA_TENSAO_MIN_OP_V) {
        _tensao_baixa_ativa = true;
        _evento_pendente = EVT_TENSAO_BAIXA;
        snprintf(_detalhe, sizeof(_detalhe), "tensao %.2fV < min %.2fV",
                 _tensao_v, (float)ENERGIA_TENSAO_MIN_OP_V);
        return;
    }
    if (_tensao_baixa_ativa &&
        _tensao_v > ENERGIA_TENSAO_MIN_OP_V + ENERGIA_HISTERESE_V) {
        _tensao_baixa_ativa = false;
    }

    // Oscilacao: variacao entre amostras acima do limite -> registra evento
    // com timestamp (o timestamp e adicionado por quem publica o evento).
    float delta = fabsf(_tensao_v - _tensao_ant_v);
    if (delta >= ENERGIA_TENSAO_OSCILACAO_V) {
        _evento_pendente = EVT_TENSAO_OSCILACAO;
        snprintf(_detalhe, sizeof(_detalhe), "oscilacao %.2fV (%.2f->%.2f)",
                 delta, _tensao_ant_v, _tensao_v);
    }
}

EventoEnergia EnergyMonitor::consumir_evento() {
    EventoEnergia e = _evento_pendente;
    _evento_pendente = EVT_NENHUM;
    return e;
}

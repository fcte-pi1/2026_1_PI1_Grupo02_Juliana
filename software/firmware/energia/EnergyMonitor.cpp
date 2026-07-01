#include "EnergyMonitor.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

EnergyMonitor::EnergyMonitor()
    : _ultima_amostra_ms(0),
      _tensao_v(0.0f),
      _tensao_ant_v(0.0f),
      _corrente_a(0.0f),
      _bateria_pct(0),
      _consumo_wh(0.0f),
      _tensao_baixa_ativa(false),
      _bat_baixa_ativa(false),
      _bat_critica_ativa(false),
      _consumo_alto_ativa(false),
      _fila_ini(0),
      _fila_fim(0) {
    _detalhe[0] = '\0';
}

void EnergyMonitor::inicializar() {
    voltage_sensor_init(&_vsensor, ENERGIA_TENSAO_GPIO, ENERGIA_TENSAO_DIVIDER_RATIO);
    current_sensor_init(&_isensor, ENERGIA_CORRENTE_GPIO,
                        ENERGIA_CORRENTE_SENS_MV_A, ENERGIA_CORRENTE_ZERO_MV);
    // Calibra o zero do Hall (deve ser chamado com a carga de potencia desligada).
    current_sensor_calibrate_zero(&_isensor);

    // Primeira leitura pra semear o valor anterior (evita falso "oscilacao" no boot).
    _tensao_v     = voltage_sensor_read_v(&_vsensor);
    _tensao_ant_v = _tensao_v;
    _corrente_a   = current_sensor_read_a(&_isensor);
    _bateria_pct  = tensao_para_pct(_tensao_v);
}

void EnergyMonitor::atualizar(uint32_t agora_ms) {
    if (agora_ms - _ultima_amostra_ms < ENERGIA_AMOSTRAGEM_MS)
        return;

    // dt real entre amostras; 0 na primeira integracao (sem intervalo valido).
    uint32_t dt_ms = (_ultima_amostra_ms == 0) ? 0 : (agora_ms - _ultima_amostra_ms);
    _ultima_amostra_ms = agora_ms;

    _tensao_ant_v = _tensao_v;
    _tensao_v     = voltage_sensor_read_v(&_vsensor);
    _corrente_a   = current_sensor_read_a(&_isensor);
    _bateria_pct  = tensao_para_pct(_tensao_v);

    // HU10: integra energia = potencia * tempo. Usa modulo da corrente pra nao
    // depender do sentido de montagem do sensor no caminho de potencia.
    if (dt_ms > 0) {
        float potencia_w = _tensao_v * fabsf(_corrente_a);
        _consumo_wh += potencia_w * (dt_ms / 3600000.0f); // ms -> h
    }

    _avaliar_tensao();
    _avaliar_bateria();
    _avaliar_consumo();
}

void EnergyMonitor::resetar_corrida() {
    _consumo_wh = 0.0f;
    _consumo_alto_ativa = false;
}

// HU16 - conversao linear tensao -> percentual, clampeada em [0, 100].
// LiPo tem curva de descarga nao-linear; a aproximacao linear entre vazia e
// cheia e suficiente pro micromouse e fica calibravel via config.h.
int EnergyMonitor::tensao_para_pct(float v) {
    const float vazia = ENERGIA_BAT_TENSAO_VAZIA_V;
    const float cheia = ENERGIA_BAT_TENSAO_CHEIA_V;
    if (v <= vazia) return 0;
    if (v >= cheia) return 100;
    return (int)lroundf((v - vazia) / (cheia - vazia) * 100.0f);
}

// HU21 - avalia queda de tensao (com histerese) e oscilacao brusca.
void EnergyMonitor::_avaliar_tensao() {
    if (!_tensao_baixa_ativa && _tensao_v < ENERGIA_TENSAO_MIN_OP_V) {
        _tensao_baixa_ativa = true;
        char d[64];
        snprintf(d, sizeof(d), "tensao %.2fV < min %.2fV",
                 _tensao_v, (float)ENERGIA_TENSAO_MIN_OP_V);
        _enfileirar(EVT_TENSAO_BAIXA, d);
    } else if (_tensao_baixa_ativa &&
               _tensao_v > ENERGIA_TENSAO_MIN_OP_V + ENERGIA_HISTERESE_V) {
        _tensao_baixa_ativa = false;
    }

    float delta = fabsf(_tensao_v - _tensao_ant_v);
    if (delta >= ENERGIA_TENSAO_OSCILACAO_V) {
        char d[64];
        snprintf(d, sizeof(d), "oscilacao %.2fV (%.2f->%.2f)",
                 delta, _tensao_ant_v, _tensao_v);
        _enfileirar(EVT_TENSAO_OSCILACAO, d);
    }
}

// HU16 - alerta de bateria fraca (20%) e critica (5%), ambos com histerese.
void EnergyMonitor::_avaliar_bateria() {
    if (!_bat_critica_ativa && _bateria_pct <= ENERGIA_BAT_CRITICO_PCT) {
        _bat_critica_ativa = true;
        char d[64];
        snprintf(d, sizeof(d), "bateria critica %d%% (%.2fV)", _bateria_pct, _tensao_v);
        _enfileirar(EVT_BATERIA_CRITICA, d);
    } else if (_bat_critica_ativa &&
               _bateria_pct > ENERGIA_BAT_CRITICO_PCT + ENERGIA_HISTERESE_PCT) {
        _bat_critica_ativa = false;
    }

    if (!_bat_baixa_ativa && _bateria_pct <= ENERGIA_BAT_ALERTA_PCT) {
        _bat_baixa_ativa = true;
        char d[64];
        snprintf(d, sizeof(d), "bateria baixa %d%% (%.2fV)", _bateria_pct, _tensao_v);
        _enfileirar(EVT_BATERIA_BAIXA, d);
    } else if (_bat_baixa_ativa &&
               _bateria_pct > ENERGIA_BAT_ALERTA_PCT + ENERGIA_HISTERESE_PCT) {
        _bat_baixa_ativa = false;
    }
}

// HU10 - alerta ao acumular 80% da capacidade da bateria. So volta a armar
// depois de resetar_corrida() (o consumo e monotonico dentro de uma corrida).
void EnergyMonitor::_avaliar_consumo() {
    float limiar_wh = (ENERGIA_CONSUMO_ALERTA_PCT / 100.0f) * ENERGIA_BAT_CAPACIDADE_WH;
    if (!_consumo_alto_ativa && _consumo_wh >= limiar_wh) {
        _consumo_alto_ativa = true;
        char d[64];
        snprintf(d, sizeof(d), "consumo %.3fWh >= %d%% de %.2fWh",
                 _consumo_wh, ENERGIA_CONSUMO_ALERTA_PCT, (float)ENERGIA_BAT_CAPACIDADE_WH);
        _enfileirar(EVT_CONSUMO_ALTO, d);
    }
}

// --- Fila de eventos ---------------------------------------------------------

void EnergyMonitor::_enfileirar(EventoEnergia tipo, const char* detalhe) {
    int prox = (_fila_fim + 1) % EVT_FILA_MAX;
    if (prox == _fila_ini) return; // fila cheia: descarta o mais novo (raro)
    _fila[_fila_fim].tipo = tipo;
    strncpy(_fila[_fila_fim].detalhe, detalhe, sizeof(_fila[_fila_fim].detalhe) - 1);
    _fila[_fila_fim].detalhe[sizeof(_fila[_fila_fim].detalhe) - 1] = '\0';
    _fila_fim = prox;
}

EventoEnergia EnergyMonitor::consumir_evento() {
    if (_fila_ini == _fila_fim) {
        _detalhe[0] = '\0';
        return EVT_NENHUM;
    }
    EventoEnergia tipo = _fila[_fila_ini].tipo;
    strncpy(_detalhe, _fila[_fila_ini].detalhe, sizeof(_detalhe) - 1);
    _detalhe[sizeof(_detalhe) - 1] = '\0';
    _fila_ini = (_fila_ini + 1) % EVT_FILA_MAX;
    return tipo;
}

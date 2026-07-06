#include "DistanceSensors.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

DistanceSensors::DistanceSensors()
    : _hc_dir(DIST_HC_DIR_TRIG, DIST_HC_DIR_ECHO, DIST_HC_TIMEOUT_US),
      _ultima_leitura_ms(0),
      _primeira(true),
      _frente_ok(false),
      _esq_ok(false),
      _colisao(false),
      _dir_colisao(DIST_N_DIRECOES) {
    // Zera as structs POD dos sensores antes do init (defesa contra uso
    // acidental de atualizar() antes de inicializar()).
    memset(&_vl_frente, 0, sizeof(_vl_frente));
    memset(&_vl_esq,    0, sizeof(_vl_esq));
    for (int i = 0; i < DIST_N_DIRECOES; i++)
        _dist[i] = DIST_INVALIDA_CM;
}

// Configura um barramento I2C. ATENCAO: i2c_init faz reset de hardware do
// periferico - NAO e idempotente. Num barramento com dono externo (ex. o
// MPU6050 no i2c0), quem chama i2c_init por ultimo ganha; por isso o i2c0 so e
// inicializado aqui quando este modulo e o dono (ver inicializar()).
static void _init_barramento(i2c_inst_t* port, uint sda, uint scl) {
    i2c_init(port, DIST_I2C_FREQ_HZ);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

void DistanceSensors::inicializar(bool i2c0_ja_inicializado) {
    // i2c1 e exclusivo deste modulo: sempre inicializa.
    _init_barramento(DIST_VL_FRENTE_I2C, DIST_VL_FRENTE_SDA, DIST_VL_FRENTE_SCL);
    // i2c0 e compartilhado com o MPU6050: so inicializa se formos o dono.
    if (!i2c0_ja_inicializado)
        _init_barramento(DIST_VL_ESQ_I2C, DIST_VL_ESQ_SDA, DIST_VL_ESQ_SCL);

    // VL53L0X frente: checa init e budget (CWE-252: nao ignorar retorno de status).
    _frente_ok = vl53l0x_init(&_vl_frente, DIST_VL_FRENTE_I2C, true);
    if (_frente_ok) {
        if (!vl53l0x_set_measurement_timing_budget(&_vl_frente, DIST_VL_TIMING_BUDGET_US))
            _frente_ok = false;
        // Limita o pior caso de stall (io_timeout do driver e ~500ms por default).
        _vl_frente.io_timeout_ms = DIST_VL_IO_TIMEOUT_MS;
    }

    _esq_ok = vl53l0x_init(&_vl_esq, DIST_VL_ESQ_I2C, true);
    if (_esq_ok) {
        if (!vl53l0x_set_measurement_timing_budget(&_vl_esq, DIST_VL_TIMING_BUDGET_US))
            _esq_ok = false;
        _vl_esq.io_timeout_ms = DIST_VL_IO_TIMEOUT_MS;
    }

    _hc_dir.inicializar();

    // Diagnostico no serial: sensor mudo no boot fica visivel em vez de virar
    // um "--" silencioso na direcao.
    if (!_frente_ok) printf("[SENSORES] VL53L0X FRENTE nao respondeu no init\r\n");
    if (!_esq_ok)    printf("[SENSORES] VL53L0X ESQUERDA nao respondeu no init\r\n");
}

bool DistanceSensors::sensor_ok(DirecaoDist d) const {
    if (d == DIR_FRENTE)   return _frente_ok;
    if (d == DIR_ESQUERDA) return _esq_ok;
    return true; // HC-SR04 nao tem init verificavel
}

float DistanceSensors::_ler_vl(VL53L0X* s) {
    uint16_t mm = vl53l0x_read_range_single_millimeters(s);
    if (mm == 65535 || s->did_timeout)
        return DIST_INVALIDA_CM;
    return mm / 10.0f;
}

void DistanceSensors::atualizar(uint32_t agora_ms) {
    if (!_primeira && (agora_ms - _ultima_leitura_ms) < DIST_INTERVALO_MS)
        return;
    _ultima_leitura_ms = agora_ms;
    _primeira = false;

    // Sensor que falhou no init nao e lido (evita I/O inutil e leitura de lixo).
    _dist[DIR_FRENTE]   = _frente_ok ? _ler_vl(&_vl_frente) : DIST_INVALIDA_CM;
    _dist[DIR_ESQUERDA] = _esq_ok    ? _ler_vl(&_vl_esq)    : DIST_INVALIDA_CM;
    _dist[DIR_DIREITA]  = _hc_dir.medir_cm();

    _avaliar_colisao();
}

float DistanceSensors::distancia_cm(DirecaoDist d) const {
    if (d < 0 || d >= DIST_N_DIRECOES)
        return DIST_INVALIDA_CM;
    return _dist[d];
}

// HU04 - alerta de colisao iminente: menor distancia valida abaixo do limiar.
// Nota: o HC-SR04 (direita) nao mede abaixo de ~2cm, entao a colisao iminente
// (< 2cm) frontal/lateral e coberta pelos VL53L0X; a direita tem esse ponto
// cego por limitacao fisica do ultrassom (documentado em perifericos.md).
void DistanceSensors::_avaliar_colisao() {
    _colisao = false;
    _dir_colisao = DIST_N_DIRECOES;
    float menor = DIST_COLISAO_CM;

    for (int i = 0; i < DIST_N_DIRECOES; i++) {
        float d = _dist[i];
        // Leitura invalida (negativa) nao conta como colisao.
        if (d >= 0.0f && d < menor) {
            menor = d;
            _colisao = true;
            _dir_colisao = (DirecaoDist)i;
        }
    }
}

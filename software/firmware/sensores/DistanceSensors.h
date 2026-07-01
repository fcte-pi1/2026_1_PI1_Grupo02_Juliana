#ifndef DISTANCE_SENSORS_H
#define DISTANCE_SENSORS_H

#include "pico/stdlib.h"
#include "sensores_config.h"
#include "HCSR04.h"

// O driver do VL53L0X e C puro: linkagem C ao incluir daqui (C++).
extern "C" {
#include "vl53l0x.h"
}

// Direcoes medidas pelo array de sensores.
enum DirecaoDist {
    DIR_FRENTE = 0,
    DIR_ESQUERDA,
    DIR_DIREITA,
    DIST_N_DIRECOES
};

// HU04 - medicao de distancia em relacao as paredes.
// Agrega os 3 sensores de distancia (2x VL53L0X + 1x HC-SR04), amostra a cada
// DIST_INTERVALO_MS e expoe a distancia por direcao e o alerta de colisao
// iminente. Nao mexe em mapa nem navegacao (isso e HU03/HU08).
class DistanceSensors {
public:
    DistanceSensors();

    // Inicializa os barramentos I2C, os dois VL53L0X e o HC-SR04. Chamar no boot.
    // i2c0_ja_inicializado: passe true quando outro modulo (ex. MPU6050) ja e o
    // dono do i2c0 e chamou i2c_init nele. Nesse caso este modulo NAO reinicia o
    // i2c0 (i2c_init faz reset de hardware e derrubaria a config do outro dono);
    // so configura os pinos e o sensor. O i2c1 e sempre exclusivo deste modulo.
    void inicializar(bool i2c0_ja_inicializado = false);

    // Saude do sensor apos inicializar(): false se o VL53L0X da direcao nao
    // respondeu no boot. HC-SR04 (direita) nao tem init verificavel: sempre true.
    bool sensor_ok(DirecaoDist d) const;

    // Le os sensores respeitando DIST_INTERVALO_MS (nao bloqueia entre amostras).
    // Chamar a cada iteracao do loop de navegacao.
    void atualizar(uint32_t agora_ms);

    // Ultima distancia lida na direcao, em cm. DIST_INVALIDA_CM se a leitura
    // falhou (timeout ou fora de range).
    float distancia_cm(DirecaoDist d) const;

    // true se alguma direcao com leitura valida esta abaixo de DIST_COLISAO_CM.
    bool colisao_iminente() const { return _colisao; }

    // Direcao da colisao mais proxima, ou DIST_N_DIRECOES se nao ha colisao.
    DirecaoDist direcao_colisao() const { return _dir_colisao; }

private:
    VL53L0X _vl_frente;
    VL53L0X _vl_esq;
    HCSR04  _hc_dir;

    uint32_t    _ultima_leitura_ms;
    bool        _primeira;
    bool        _frente_ok;   // VL53 frente respondeu no init
    bool        _esq_ok;      // VL53 esquerda respondeu no init
    float       _dist[DIST_N_DIRECOES];
    bool        _colisao;
    DirecaoDist _dir_colisao;

    float _ler_vl(VL53L0X* s);   // mm -> cm, DIST_INVALIDA_CM em timeout
    void  _avaliar_colisao();
};

#endif // DISTANCE_SENSORS_H

#ifndef HCSR04_H
#define HCSR04_H

#include "pico/stdlib.h"

// Driver do ultrassom HC-SR04 com pinos por instancia (permite mais de um
// sensor no robo). A logica de disparo/eco espelha o driver de funcao basica
// do projeto (funcoes_basicas_hcsr04), parametrizada aqui pra reuso.
//
// Limitacao fisica do sensor: nao mede abaixo de ~2cm; leituras nessa faixa
// retornam invalido. Por isso a colisao frontal critica (< 2cm) fica a cargo
// dos VL53L0X, nao do HC-SR04.
class HCSR04 {
public:
    HCSR04(uint pino_trig, uint pino_echo, uint32_t timeout_us);

    void inicializar();

    // Faz uma medicao e retorna a distancia em cm, ou DIST_INVALIDA_CM
    // (negativo) em timeout ou leitura fora de range.
    float medir_cm();

private:
    uint     _trig;
    uint     _echo;
    uint32_t _timeout_us;

    void disparar_burst();
    bool aguardar_echo(bool nivel);  // aguarda ECHO chegar em 'nivel', com timeout
};

#endif // HCSR04_H

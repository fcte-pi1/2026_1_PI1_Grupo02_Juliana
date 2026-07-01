#include "HCSR04.h"
#include "hardware/gpio.h"
#include "sensores_config.h"

// Velocidade do som: ~58us por cm (ida e volta). Mesmo fator do driver base.
static constexpr float VELOCIDADE_SOM_US_CM = 58.0f;

HCSR04::HCSR04(uint pino_trig, uint pino_echo, uint32_t timeout_us)
    : _trig(pino_trig), _echo(pino_echo), _timeout_us(timeout_us) {}

void HCSR04::inicializar() {
    gpio_init(_trig);
    gpio_init(_echo);
    gpio_set_dir(_trig, GPIO_OUT);
    gpio_set_dir(_echo, GPIO_IN);
    // Pull-down no ECHO: se o sensor estiver desconectado, o pino nao flutua e
    // da leitura invalida em vez de uma largura de pulso espuria (falsa).
    gpio_pull_down(_echo);
    gpio_put(_trig, 0);
    sleep_ms(50);
}

void HCSR04::disparar_burst() {
    gpio_put(_trig, 0);
    sleep_us(2);
    gpio_put(_trig, 1);
    sleep_us(10);   // 10us conforme datasheet
    gpio_put(_trig, 0);
}

bool HCSR04::aguardar_echo(bool nivel) {
    uint32_t inicio = time_us_32();
    while (gpio_get(_echo) != (nivel ? 1 : 0)) {
        if ((time_us_32() - inicio) > _timeout_us)
            return false;
    }
    return true;
}

float HCSR04::medir_cm() {
    disparar_burst();

    // Espera a subida do ECHO (inicio do pulso).
    if (!aguardar_echo(true))
        return DIST_INVALIDA_CM;

    uint32_t inicio = time_us_32();
    // Espera a descida (fim do pulso); a largura e proporcional a distancia.
    while (gpio_get(_echo) == 1) {
        if ((time_us_32() - inicio) > _timeout_us)
            return DIST_INVALIDA_CM;
    }
    uint32_t largura_us = time_us_32() - inicio;

    float distancia = largura_us / VELOCIDADE_SOM_US_CM;

    // Fora do range confiavel do sensor -> invalido.
    if (distancia < 2.0f || distancia > 400.0f)
        return DIST_INVALIDA_CM;

    return distancia;
}

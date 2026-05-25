#include "hcsr04.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

static void trigger_burst(void)
{
    // Pulso TTL de 10 µs no pino TRIG
    gpio_put(HCSR04_TRIG_PIN, 0);
    sleep_us(2);
    gpio_put(HCSR04_TRIG_PIN, 1);
    sleep_us(HCSR04_TRIG_PULSE_US);
    gpio_put(HCSR04_TRIG_PIN, 0);
}

// Aguarda o pino ECHO atingir o estado desejado e retorna o tick em µs.
// Retorna 0 em caso de timeout.
static uint32_t wait_echo_edge(bool target_level, uint32_t timeout_us)
{
    uint32_t inicio = time_us_32();
    while (gpio_get(HCSR04_ECHO_PIN) != target_level) {
        if ((time_us_32() - inicio) > timeout_us) {
            return 0;
        }
    }
    return time_us_32();
}

static int compare_floats(const void *a, const void *b)
{
    float fa = *(const float *)a;
    float fb = *(const float *)b;
    return (fa > fb) - (fa < fb);
}

void hcsr04_init(void)
{
    gpio_init(HCSR04_TRIG_PIN);
    gpio_set_dir(HCSR04_TRIG_PIN, GPIO_OUT);
    gpio_put(HCSR04_TRIG_PIN, 0);

    gpio_init(HCSR04_ECHO_PIN);
    gpio_set_dir(HCSR04_ECHO_PIN, GPIO_IN);

    sleep_ms(HCSR04_SETTLE_MS);
    printf("[HCSR04] Inicializado (TRIG=GP%d, ECHO=GP%d)\n",
           HCSR04_TRIG_PIN, HCSR04_ECHO_PIN);
}

float hcsr04_medir_distancia_cm(void)
{
    trigger_burst();

    uint32_t t_rise = wait_echo_edge(true, HCSR04_ECHO_TIMEOUT_US);
    if (t_rise == 0) {
        return HCSR04_INVALID_READING;
    }

    uint32_t t_fall = wait_echo_edge(false, HCSR04_ECHO_TIMEOUT_US);
    if (t_fall == 0) {
        return HCSR04_INVALID_READING;
    }

    uint32_t pulso_us = t_fall - t_rise;
    float distancia_cm = (float)pulso_us / HCSR04_US_PER_CM;

    if (distancia_cm < HCSR04_MIN_DIST_CM || distancia_cm > HCSR04_MAX_DIST_CM) {
        return HCSR04_INVALID_READING;
    }

    return distancia_cm;
}

float hcsr04_medir_filtrado_cm(uint8_t n_amostras)
{
    if (n_amostras < 3)  n_amostras = 3;
    if (n_amostras > 15) n_amostras = 15;

    float amostras[15];
    uint8_t validas = 0;

    for (uint8_t i = 0; i < n_amostras; i++) {
        float d = hcsr04_medir_distancia_cm();
        if (d != HCSR04_INVALID_READING) {
            amostras[validas++] = d;
        }
        sleep_ms(HCSR04_MIN_CYCLE_MS);
    }

    if (validas == 0) return HCSR04_INVALID_READING;

    qsort(amostras, validas, sizeof(float), compare_floats);
    return amostras[validas / 2];
}

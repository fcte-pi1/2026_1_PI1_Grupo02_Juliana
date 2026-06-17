#include "encoder.h"
#include "hardware/gpio.h"

// O RP2040 tem um unico callback de IRQ de GPIO por nucleo. Para suportar
// varios encoders (ate ENCODER_MAX), guardamos os encoders registrados num
// vetor estatico e despachamos no callback compartilhado.
static encoder_t *registry[ENCODER_MAX];
static int registry_len = 0;

// Tabela de transicao para quadratura x4.
// indice = (estado_anterior << 2) | estado_atual, sendo estado = (A<<1)|B.
// Valor: +1 (avanco), -1 (retrocesso), 0 (sem mudanca ou transicao invalida).
static const int8_t QUAD_TABLE[16] = {
    0, -1, +1,  0,
   +1,  0,  0, -1,
   -1,  0,  0, +1,
    0, +1, -1,  0
};

static inline uint8_t read_state(encoder_t *enc) {
    return (uint8_t)((gpio_get(enc->pin_a) << 1) | gpio_get(enc->pin_b));
}

// Callback unico: atualiza o encoder cujo canal gerou o evento.
static void encoder_gpio_callback(uint gpio, uint32_t events) {
    (void)events;
    for (int i = 0; i < registry_len; i++) {
        encoder_t *enc = registry[i];
        if (enc->pin_a == gpio || enc->pin_b == gpio) {
            uint8_t state = read_state(enc);
            uint8_t index = (uint8_t)((enc->last_state << 2) | state);
            enc->count += QUAD_TABLE[index & 0x0F];
            enc->last_state = state;
        }
    }
}

void encoder_init(encoder_t *enc, uint pin_a, uint pin_b, uint16_t gear_ratio) {
    enc->pin_a = pin_a;
    enc->pin_b = pin_b;
    enc->count = 0;
    enc->counts_per_rev = (float)ENCODER_PPR_MOTOR * ENCODER_QUAD_MULT * gear_ratio;
    enc->last_sample_count = 0;
    enc->last_sample_us = time_us_64();

    gpio_init(pin_a);
    gpio_init(pin_b);
    gpio_set_dir(pin_a, GPIO_IN);
    gpio_set_dir(pin_b, GPIO_IN);
    gpio_pull_up(pin_a);
    gpio_pull_up(pin_b);

    enc->last_state = read_state(enc);

    if (registry_len < ENCODER_MAX) {
        registry[registry_len++] = enc;
    }

    // A primeira chamada registra o callback compartilhado; as demais apenas
    // habilitam a IRQ nos novos pinos.
    gpio_set_irq_enabled_with_callback(pin_a, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true, &encoder_gpio_callback);
    gpio_set_irq_enabled(pin_b, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

void encoder_reset(encoder_t *enc) {
    enc->count = 0;
    enc->last_sample_count = 0;
    enc->last_sample_us = time_us_64();
}

int32_t encoder_get_count(encoder_t *enc) {
    return enc->count;
}

float encoder_get_revolutions(encoder_t *enc) {
    return enc->count / enc->counts_per_rev;
}

float encoder_get_rpm(encoder_t *enc) {
    uint64_t now = time_us_64();
    int32_t count = enc->count;

    int32_t d_count = count - enc->last_sample_count;
    uint64_t d_us = now - enc->last_sample_us;

    enc->last_sample_count = count;
    enc->last_sample_us = now;

    if (d_us == 0) {
        return 0.0f;
    }

    // voltas no intervalo = d_count / counts_per_rev
    // RPM = voltas / minuto = (d_count / counts_per_rev) / (d_us / 60e6)
    float revs = d_count / enc->counts_per_rev;
    return revs * (60000000.0f / (float)d_us);
}

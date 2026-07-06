#include "Motor.h"
#include "hardware/clocks.h"

// O wrap=255 faz o nível PWM coincidir diretamente com o valor de velocidade
// (-255..255), sem reescala. A frequência de 20 kHz fica fora da faixa audível
// e o clkdiv é calculado dinamicamente a partir do clock do sistema.
#define PWM_WRAP     255u
#define PWM_FREQ_HZ  20000u

Motor::Motor(uint pwm, uint dir1, uint dir2)
    : pinPWM(pwm), pinDir1(dir1), pinDir2(dir2) {
    pwm_slice_num = pwm_gpio_to_slice_num(pinPWM);
}

void Motor::inicializar() {
    gpio_init(pinDir1);
    gpio_init(pinDir2);
    gpio_set_dir(pinDir1, GPIO_OUT);
    gpio_set_dir(pinDir2, GPIO_OUT);

    gpio_set_function(pinPWM, GPIO_FUNC_PWM);

    float clkdiv = (float)clock_get_hz(clk_sys) / (PWM_FREQ_HZ * (PWM_WRAP + 1));
    pwm_set_clkdiv(pwm_slice_num, clkdiv);
    pwm_set_wrap(pwm_slice_num, PWM_WRAP);
    pwm_set_chan_level(pwm_slice_num, pwm_gpio_to_channel(pinPWM), 0);
    pwm_set_enabled(pwm_slice_num, true);

    parar();
}

void Motor::setVelocidade(int velocidade) {
    if (velocidade > 255)  velocidade =  255;
    if (velocidade < -255) velocidade = -255;

    if (velocidade > 0) {
        gpio_put(pinDir1, 1);
        gpio_put(pinDir2, 0);
        pwm_set_gpio_level(pinPWM, (uint16_t)velocidade);
    } else if (velocidade < 0) {
        gpio_put(pinDir1, 0);
        gpio_put(pinDir2, 1);
        pwm_set_gpio_level(pinPWM, (uint16_t)(-velocidade));
    } else {
        parar();
    }
}

void Motor::parar() {
    gpio_put(pinDir1, 0);
    gpio_put(pinDir2, 0);
    pwm_set_gpio_level(pinPWM, 0);
}

void Motor::frear() {
    gpio_put(pinDir1, 1);
    gpio_put(pinDir2, 1);
    pwm_set_gpio_level(pinPWM, PWM_WRAP);
}
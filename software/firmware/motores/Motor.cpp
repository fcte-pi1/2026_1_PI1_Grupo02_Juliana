#include "Motor.h"
#include <cstdlib>

Motor::Motor(uint pwm, uint dir1, uint dir2) 
    : pinPWM(pwm), pinDir1(dir1), pinDir2(dir2) {
    pwm_slice_num = pwm_gpio_to_slice_num(pinPWM);
}

void Motor::inicializar() {
    // Inicializa os pinos de direção como saídas GPIO simples
    gpio_init(pinDir1);
    gpio_init(pinDir2);
    gpio_set_dir(pinDir1, GPIO_OUT);
    gpio_set_dir(pinDir2, GPIO_OUT);
    
    // Inicializa o pino PWM
    gpio_init(pinPWM);
    gpio_set_function(pinPWM, GPIO_FUNC_PWM);
    
    // Configura PWM: frequência de ~1 kHz (típico para motores)
    // Clock base = 125 MHz, divider = 125 → 1 MHz
    pwm_set_clkdiv(pwm_slice_num, 125.0f);
    // wrap = 1000 → frequência de 1 kHz
    pwm_set_wrap(pwm_slice_num, 1000);
    pwm_set_enabled(pwm_slice_num, true);
    
    parar();
}

void Motor::setVelocidade(int velocidade) {
    // Converte valor 0-255 para 0-1000 (range do PWM)
    // Limita o valor de entrada
    if (velocidade > 255) velocidade = 255;
    if (velocidade < -255) velocidade = -255;
    
    uint16_t pwm_value = (abs(velocidade) * 1000) / 255;
    
    if (velocidade > 0) {
        // Frente
        gpio_put(pinDir1, 1);
        gpio_put(pinDir2, 0);
        pwm_set_gpio_level(pinPWM, pwm_value);
    } else if (velocidade < 0) {
        // Trás
        gpio_put(pinDir1, 0);
        gpio_put(pinDir2, 1);
        pwm_set_gpio_level(pinPWM, pwm_value);
    } else {
        parar();
    }
}

void Motor::parar() {
    gpio_put(pinDir1, 0);
    gpio_put(pinDir2, 0);
    pwm_set_gpio_level(pinPWM, 0);
}
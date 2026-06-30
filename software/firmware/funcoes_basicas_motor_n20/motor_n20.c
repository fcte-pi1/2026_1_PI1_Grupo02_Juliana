#include "motor_n20.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

// O PWM conta de 0 ate PWM_WRAP. Usando WRAP = MOTOR_N20_SPEED_MAX (255) o
// valor de velocidade vira diretamente o nivel do PWM, sem reescala.
#define PWM_WRAP      MOTOR_N20_SPEED_MAX
#define PWM_FREQ_HZ   20000u  // 20 kHz: silencioso e adequado para a ponte H

static void set_dir_forward(motor_n20_t *motor) {
    gpio_put(motor->pin_in1, 1);
    gpio_put(motor->pin_in2, 0);
}

static void set_dir_reverse(motor_n20_t *motor) {
    gpio_put(motor->pin_in1, 0);
    gpio_put(motor->pin_in2, 1);
}

void motor_n20_init(motor_n20_t *motor, uint pin_pwm, uint pin_in1, uint pin_in2) {
    motor->pin_pwm = pin_pwm;
    motor->pin_in1 = pin_in1;
    motor->pin_in2 = pin_in2;

    // Pinos de direcao como saida digital
    gpio_init(pin_in1);
    gpio_init(pin_in2);
    gpio_set_dir(pin_in1, GPIO_OUT);
    gpio_set_dir(pin_in2, GPIO_OUT);

    // Pino de PWM
    gpio_set_function(pin_pwm, GPIO_FUNC_PWM);
    motor->pwm_slice = pwm_gpio_to_slice_num(pin_pwm);
    motor->pwm_channel = pwm_gpio_to_channel(pin_pwm);

    // clkdiv = f_sys / (f_pwm * (WRAP + 1))
    float clkdiv = (float)clock_get_hz(clk_sys) / (PWM_FREQ_HZ * (PWM_WRAP + 1));
    pwm_set_clkdiv(motor->pwm_slice, clkdiv);
    pwm_set_wrap(motor->pwm_slice, PWM_WRAP);
    pwm_set_chan_level(motor->pwm_slice, motor->pwm_channel, 0);
    pwm_set_enabled(motor->pwm_slice, true);

    motor_n20_stop(motor);
}

void motor_n20_set_speed(motor_n20_t *motor, int speed) {
    if (speed > MOTOR_N20_SPEED_MAX)  speed =  MOTOR_N20_SPEED_MAX;
    if (speed < -MOTOR_N20_SPEED_MAX) speed = -MOTOR_N20_SPEED_MAX;

    if (speed > 0) {
        set_dir_forward(motor);
        pwm_set_chan_level(motor->pwm_slice, motor->pwm_channel, speed);
    } else if (speed < 0) {
        set_dir_reverse(motor);
        pwm_set_chan_level(motor->pwm_slice, motor->pwm_channel, -speed);
    } else {
        motor_n20_stop(motor);
    }
}

void motor_n20_stop(motor_n20_t *motor) {
    gpio_put(motor->pin_in1, 0);
    gpio_put(motor->pin_in2, 0);
    pwm_set_chan_level(motor->pwm_slice, motor->pwm_channel, 0);
}

void motor_n20_brake(motor_n20_t *motor) {
    gpio_put(motor->pin_in1, 1);
    gpio_put(motor->pin_in2, 1);
    pwm_set_chan_level(motor->pwm_slice, motor->pwm_channel, PWM_WRAP);
}

#ifndef MOTOR_N20_H
#define MOTOR_N20_H

#include "pico/stdlib.h"

// Faixa de velocidade aceita por motor_n20_set_speed().
// O sinal indica o sentido (positivo = frente, negativo = re), o modulo
// indica a intensidade do PWM. Mantem a mesma convencao da classe Motor ja
// usada no firmware (-255 a 255).
#define MOTOR_N20_SPEED_MAX 255

// Controle de um motor N20 (ASLONG JGA12-N10B) atraves de uma ponte H
// (ex.: TB6612FNG ou DRV8833). O RP2040 NAO aciona o motor diretamente:
// os fios M+ (vermelho) e M- (branco) do motor vao para a saida da ponte H,
// e os pinos abaixo controlam a entrada da ponte.
//   pin_pwm -> PWMx  (velocidade, sinal PWM)
//   pin_in1 -> xIN1  (sentido)
//   pin_in2 -> xIN2  (sentido)
typedef struct {
    uint pin_pwm;
    uint pin_in1;
    uint pin_in2;
    uint pwm_slice;
    uint pwm_channel;
} motor_n20_t;

// Configura os pinos de direcao e o PWM (~20 kHz, fora da faixa audivel).
void motor_n20_init(motor_n20_t *motor, uint pin_pwm, uint pin_in1, uint pin_in2);

// Define a velocidade do motor de -MOTOR_N20_SPEED_MAX a +MOTOR_N20_SPEED_MAX.
// Positivo gira para frente, negativo para tras, 0 deixa em roda-livre (coast).
void motor_n20_set_speed(motor_n20_t *motor, int speed);

// Roda-livre: corta o PWM e desliga as duas entradas (IN1=IN2=0). O motor
// para por inercia.
void motor_n20_stop(motor_n20_t *motor);

// Frenagem ativa (short brake): liga as duas entradas (IN1=IN2=1) curto-
// circuitando o motor. Para mais rapido que motor_n20_stop().
void motor_n20_brake(motor_n20_t *motor);

#endif

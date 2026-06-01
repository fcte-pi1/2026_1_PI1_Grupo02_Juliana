#include "PID.h"

PID::PID(float kp, float ki, float kd, float limite) 
    : kp(kp), ki(ki), kd(kd), limiteSaida(limite), erroAnterior(0), somaErro(0) {}

float PID::calcular(float setpoint, float valorAtual, float deltaTime) {
    if (deltaTime <= 0.0f) return 0.0f;

    float erro = setpoint - valorAtual;
    somaErro += erro * deltaTime;
    
    // Derivada (taxa de variação do erro)
    float derivada = (erro - erroAnterior) / deltaTime;
    
    // Cálculo final da saída (u(t))
    float saida = (kp * erro) + (ki * somaErro) + (kd * derivada);
    
    erroAnterior = erro;

    // Saturação (limita o PWM máximo)
    if (saida > limiteSaida) return limiteSaida;
    if (saida < -limiteSaida) return -limiteSaida;
    
    return saida;
}

void PID::resetar() {
    erroAnterior = 0;
    somaErro = 0;
}
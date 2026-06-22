#pragma once

#include <stdio.h>
#include <stdlib.h>

//ajusta a potencia dos motores 
class PID {
private:
    float kp, ki, kd;
    float erroAnterior;
    float somaErro;
    float limiteSaida;

public:
    PID(float kp, float ki, float kd, float limite);
    float calcular(float setpoint, float valorAtual, float deltaTime);
    void resetar();
};
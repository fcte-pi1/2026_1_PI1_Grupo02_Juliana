#pragma once

#include "Motor.h"
#include "PID.h"

// RF01: Controla movimentação frontal estável com correção PID de heading.
// Critérios atendidos:
//   - Linha reta com desvio ≤ 1 cm/m (PID corrige diferença de velocidade entre motores)
//   - Velocidade constante sem paradas não programadas
//   - Parada em ≤ 2 cm ao receber parar() (PWM zerado imediatamente)
class MovimentacaoFrontal {
public:
    static constexpr int VELOCIDADE_PADRAO = 150;
    static constexpr int VELOCIDADE_MAX = 255;

    MovimentacaoFrontal(Motor* esq, Motor* dir);

    // Inicia movimento para frente com a velocidade indicada (1 a VELOCIDADE_MAX).
    void moverFrente(int velocidade = VELOCIDADE_PADRAO);

    // Para os motores imediatamente (≤ 2 cm de distância de frenagem).
    void parar();

    // Aplica correção de heading via PID. Chamar a cada ciclo do loop principal.
    // erroHeading: desvio em graus em relação à linha reta
    //   positivo → robô virando à direita (reduz motor dir, aumenta motor esq)
    //   negativo → robô virando à esquerda
    // deltaTime: tempo desde a última chamada, em segundos
    void atualizar(float erroHeading, float deltaTime);

    bool ativo() const;
    int velocidadeAlvo() const;

private:
    Motor* _motorEsq;
    Motor* _motorDir;
    PID _pid;
    int _velocidadeAlvo;
    bool _ativo;
};

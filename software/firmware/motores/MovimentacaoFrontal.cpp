#include "MovimentacaoFrontal.h"
#include <algorithm>

// kp=2.0, ki=0.1, kd=0.5 → resposta rápida sem oscilação; limite=50 PWM
MovimentacaoFrontal::MovimentacaoFrontal(Motor* esq, Motor* dir)
    : _motorEsq(esq), _motorDir(dir),
      _pid(2.0f, 0.1f, 0.5f, 50.0f),
      _velocidadeAlvo(0), _ativo(false) {}

void MovimentacaoFrontal::moverFrente(int velocidade) {
    _velocidadeAlvo = std::max(-VELOCIDADE_MAX, std::min(VELOCIDADE_MAX, velocidade));
    _pid.resetar();
    _ativo = true;
    _motorEsq->setVelocidade(_velocidadeAlvo);
    _motorDir->setVelocidade(_velocidadeAlvo);
}

void MovimentacaoFrontal::parar() {
    _ativo = false;
    _velocidadeAlvo = 0;
    _motorEsq->parar();
    _motorDir->parar();
}

void MovimentacaoFrontal::atualizar(float erroHeading, float deltaTime) {
    if (!_ativo || deltaTime <= 0.0f) return;

    float correcao = _pid.calcular(0.0f, erroHeading, deltaTime);

    int velEsq = (int)(_velocidadeAlvo - correcao);
    int velDir = (int)(_velocidadeAlvo + correcao);

    velEsq = std::max(-VELOCIDADE_MAX, std::min(VELOCIDADE_MAX, velEsq));
    velDir = std::max(-VELOCIDADE_MAX, std::min(VELOCIDADE_MAX, velDir));

    _motorEsq->setVelocidade(velEsq);
    _motorDir->setVelocidade(velDir);
}

bool MovimentacaoFrontal::ativo() const { return _ativo; }
int MovimentacaoFrontal::velocidadeAlvo() const { return _velocidadeAlvo; }

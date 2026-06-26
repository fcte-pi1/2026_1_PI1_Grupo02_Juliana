#include "Rotacao.h"
#include <cstdlib>

Rotacao::Rotacao(Motor* esq, Motor* dir)
    : _motorEsq(esq), _motorDir(dir),
      _emRotacao(false), _inicioSetado(false),
      _orientacao(Orientacao::N),
      _inicioMs(0), _duracaoMs(0),
      _sentido(1), _anguloAlvo(0), _velocidade(VELOCIDADE_PADRAO) {}

void Rotacao::girar(int angulo, int velocidade) {
    if (angulo == 0) return;

    _anguloAlvo  = angulo;
    _sentido     = (angulo > 0) ? 1 : -1;
    _velocidade  = velocidade;
    _duracaoMs   = (uint32_t)(std::abs(angulo) * MS_POR_GRAU);
    _inicioSetado = false;
    _emRotacao   = true;

    // Motores em sentidos opostos → giro no próprio eixo (sem deslocar célula)
    // Sentido +1 (direita): esq avança, dir recua
    // Sentido -1 (esquerda): esq recua, dir avança
    _motorEsq->setVelocidade(_sentido * _velocidade);
    _motorDir->setVelocidade(-_sentido * _velocidade);
}

bool Rotacao::atualizar(uint32_t agoraMs) {
    if (!_emRotacao) return false;

    if (!_inicioSetado) {
        _inicioMs     = agoraMs;
        _inicioSetado = true;
        return true;
    }

    if (agoraMs - _inicioMs >= _duracaoMs) {
        _pararMotores();
        _atualizarOrientacao();
        _emRotacao = false;
        return false;
    }

    return true;
}

void Rotacao::_pararMotores() {
    _motorEsq->parar();
    _motorDir->parar();
}

void Rotacao::_atualizarOrientacao() {
    // N=0, E=1, S=2, W=3
    // +90° (direita) → +1 mod 4
    // -90° (esquerda) → +3 mod 4  (equivale a -1 mod 4)
    // ±180°  → +2 mod 4
    int delta;
    if (_anguloAlvo == 180 || _anguloAlvo == -180) {
        delta = 2;
    } else {
        delta = (_sentido > 0) ? 1 : 3;
    }
    _orientacao = static_cast<Orientacao>((static_cast<int>(_orientacao) + delta) % 4);
}

bool Rotacao::emRotacao() const { return _emRotacao; }

Rotacao::Orientacao Rotacao::orientacaoAtual() const { return _orientacao; }

char Rotacao::orientacaoChar() const {
    switch (_orientacao) {
        case Orientacao::N: return 'N';
        case Orientacao::E: return 'E';
        case Orientacao::S: return 'S';
        case Orientacao::W: return 'W';
        default:            return 'N';
    }
}

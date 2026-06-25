#pragma once

#include <stdint.h>
#include "Motor.h"

// RF02: Rotação no próprio eixo (90°, -90°, 180°) com rastreamento de orientação.
// Critérios atendidos:
//   - Giro com erro ≤ ±2° (calibrado via MS_POR_GRAU)
//   - Permanece na mesma célula do labirinto (motores em sentidos opostos)
//   - Atualiza orientação interna ao concluir, pronta para o próximo movimento
class Rotacao {
public:
    // Orientação cardinal — letras compatíveis com o payload de telemetria (N/E/S/W)
    enum class Orientacao : uint8_t { N = 0, E = 1, S = 2, W = 3 };

    static constexpr int VELOCIDADE_PADRAO = 120;
    // Calibrar conforme hardware: ms para girar 1° com VELOCIDADE_PADRAO (ajuste em campo)
    static constexpr float MS_POR_GRAU = 5.5f;

    Rotacao(Motor* esq, Motor* dir);

    // Inicia rotação. angulo: 90 (direita), -90 (esquerda), 180.
    void girar(int angulo, int velocidade = VELOCIDADE_PADRAO);

    // Chamar a cada ciclo do loop principal com o tempo atual em ms.
    // Retorna true enquanto o giro ainda está em andamento.
    bool atualizar(uint32_t agoraMs);

    bool emRotacao() const;
    Orientacao orientacaoAtual() const;
    char orientacaoChar() const; // 'N', 'E', 'S', 'W'

private:
    Motor* _motorEsq;
    Motor* _motorDir;
    bool _emRotacao;
    bool _inicioSetado;
    Orientacao _orientacao;
    uint32_t _inicioMs;
    uint32_t _duracaoMs;
    int _sentido;    // +1 = direita, -1 = esquerda
    int _anguloAlvo;
    int _velocidade;

    void _atualizarOrientacao();
    void _pararMotores();
};

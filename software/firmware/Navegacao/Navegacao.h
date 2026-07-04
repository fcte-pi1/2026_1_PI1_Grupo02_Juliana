#pragma once
#include <stdint.h>
#include "../FloodFill/FloodFill.h"
#include "../telemetria/MazeMapper.h"
#include "../SensorManager/SensorManager.h"
#include "../motores/MovimentacaoFrontal.h"
#include "../motores/Rotacao.h"

// ──────────────────────────────────────────────────────────────────────────────
// Navigation — máquina de estados do Micromouse
//
// Orquestra todos os outros módulos. Cada chamada a update() executa:
//   1. Lê sensores e armazena paredes no FloodFill
//   2. Recalcula pesos (BFS)
//   3. Determina melhor direção
//   4. Gira o robô se necessário
//   5. Avança uma célula
//   6. Atualiza posição lógica
//   7. Verifica transição de estado
// ──────────────────────────────────────────────────────────────────────────────

enum NavState : uint8_t {
    NAV_EXPLORE      = 0,  // mapeia labirinto em direção ao centro
    NAV_RETURN_HOME,       // retorna à origem com mapa completo
    NAV_SPEED_RUN,         // corre pelo caminho ótimo (expansão futura)
    NAV_DONE,              // chegou ao destino final
};

struct RobotPose {
    uint8_t   x;
    uint8_t   y;
    Direction heading;
};

// Posição de partida — sempre o canto (0,0), qualquer que seja o tamanho do labirinto.
#define NAV_HOME_X    0u
#define NAV_HOME_Y    0u

class Navigation {
public:
    Navigation(FloodFill&           ff,
               MazeMapper&          mapper,
               SensorManager&       sensors,
               MovimentacaoFrontal& movFrente,
               Rotacao&             rotacao);

    // Inicializa posição, FloodFill (tamanho default FF_MAZE_SIZE_MAX) e primeira rodada de FloodFill.
    void init();

    // Reconfigura para um labirinto mazeSize x mazeSize (4, 8, 16...) e reinicia a
    // exploração do zero. Usado quando o operador escolhe o tamanho do labirinto
    // (ex.: via comando MQTT vindo do front).
    void reconfigure(uint8_t mazeSize);

    // Executa um passo da navegação (chamar em loop).
    // Bloqueia enquanto o robô está em movimento físico.
    void update();

    const RobotPose& pose()   const;
    NavState         state()  const;
    bool             isDone() const;

private:
    FloodFill&           ff_;
    MazeMapper&          mapper_;
    SensorManager&       sensors_;
    MovimentacaoFrontal& movFrente_;
    Rotacao&             rotacao_;

    RobotPose pose_;
    NavState  state_;

    // Células-alvo da exploração: bloco central 2x2 (ou 1 célula, se o tamanho for ímpar).
    // Recalculado em reconfigure() a partir de ff_.size().
    uint8_t goalX_[FF_MAX_GOALS];
    uint8_t goalY_[FF_MAX_GOALS];
    uint8_t goalCount_;

    // Preenche goalX_/goalY_/goalCount_ com o bloco central do labirinto atual.
    void computeGoalCells();

    // true se (x,y) é uma das células-alvo (bloco central).
    bool isAtGoal(uint8_t x, uint8_t y) const;

    // Lê sensores e registra paredes na célula atual no FloodFill.
    void storeWalls();

    // Calcula melhor direção, gira e avança. Atualiza pose_.
    void stepTowardTarget();

    // Aplica rotação física e atualiza pose_.heading.
    void executeTurn(Direction desired);

    // Atualiza pose_.x e pose_.y conforme pose_.heading.
    void advancePosition();

    // Conversão de direção relativa (esq/dir) para direção absoluta
    Direction relLeft(Direction h)  const;
    Direction relRight(Direction h) const;
};

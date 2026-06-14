#pragma once
#include <stdint.h>
#include "../FloodFill/FloodFill.h"
#include "../telemetria/MazeMapper.h"
#include "../SensorManager/SensorManager.h"
#include "../MotorController/MotorController.h"

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

// Posições padrão do labirinto de competição Micromouse
#define NAV_HOME_X    0u
#define NAV_HOME_Y    0u
#define NAV_TARGET_X  8u   // centro do labirinto 16×16
#define NAV_TARGET_Y  8u

class Navigation {
public:
    Navigation(FloodFill&       ff,
               MazeMapper&      mapper,
               SensorManager&   sensors,
               MotorController& motors);

    // Inicializa posição, FloodFill e executa primeira rodada de FloodFill.
    void init();

    // Executa um passo da navegação (chamar em loop).
    // Bloqueia enquanto o robô está em movimento físico.
    void update();

    const RobotPose& pose()   const;
    NavState         state()  const;
    bool             isDone() const;

private:
    FloodFill&       ff_;
    MazeMapper&      mapper_;
    SensorManager&   sensors_;
    MotorController& motors_;

    RobotPose pose_;
    NavState  state_;

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

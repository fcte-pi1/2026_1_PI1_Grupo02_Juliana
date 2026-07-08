#include "Navegacao.h"
#include <stdio.h>

Navigation::Navigation(FloodFill& ff, MazeMapper& mapper,
                       SensorManager& sensors,
                       MovimentacaoFrontal& movFrente,
                       Rotacao& rotacao)
    : ff_(ff), mapper_(mapper), sensors_(sensors),
      movFrente_(movFrente), rotacao_(rotacao),
      state_(NAV_EXPLORE) {}

// ──────────────────────────────────────────────────────────────────────────────

void Navigation::init() {
    reconfigure(FF_MAZE_SIZE_MAX);
}

void Navigation::reconfigure(uint8_t mazeSize) {
    pose_ = {NAV_HOME_X, NAV_HOME_Y, DIR_NORTH};
    state_ = NAV_EXPLORE;

    ff_.init(mazeSize);
    computeGoalCells();
    ff_.recomputeMulti(goalX_, goalY_, goalCount_);

    printf("[NAV] Inicio em (%d,%d) heading=%s labirinto=%dx%d alvo_celulas=%d\r\n",
           pose_.x, pose_.y, FloodFill::dirToStr(pose_.heading),
           ff_.size(), ff_.size(), goalCount_);
}

// ──────────────────────────────────────────────────────────────────────────────
// Bloco central do labirinto: 2x2 se o lado for par (caso normal de competição),
// 1 célula se for ímpar. Ex.: size=8 → células (3,3),(3,4),(4,3),(4,4).
// ──────────────────────────────────────────────────────────────────────────────

void Navigation::computeGoalCells() {
    uint8_t n = ff_.size();
    uint8_t goalSize = (n % 2 == 0) ? 2 : 1;
    uint8_t goalMin  = (uint8_t)((n - goalSize) / 2);

    goalCount_ = 0;
    for (uint8_t dx = 0; dx < goalSize; dx++)
        for (uint8_t dy = 0; dy < goalSize; dy++) {
            goalX_[goalCount_] = (uint8_t)(goalMin + dx);
            goalY_[goalCount_] = (uint8_t)(goalMin + dy);
            goalCount_++;
        }
}

bool Navigation::isAtGoal(uint8_t x, uint8_t y) const {
    for (uint8_t i = 0; i < goalCount_; i++)
        if (goalX_[i] == x && goalY_[i] == y) return true;
    return false;
}

const RobotPose& Navigation::pose()   const { return pose_; }
NavState         Navigation::state()  const { return state_; }
bool             Navigation::isDone() const { return state_ == NAV_DONE; }

// ──────────────────────────────────────────────────────────────────────────────
// Direção relativa → absoluta:
//   esquerda  = (heading + 3) % 4
//   direita   = (heading + 1) % 4
// ──────────────────────────────────────────────────────────────────────────────

Direction Navigation::relLeft(Direction h)  const {
    return static_cast<Direction>((h + 3) % 4);
}
Direction Navigation::relRight(Direction h) const {
    return static_cast<Direction>((h + 1) % 4);
}

// ──────────────────────────────────────────────────────────────────────────────
// Lê sensores, traduz para direções absolutas e atualiza o FloodFill.
// Marca a célula atual como visitada.
// Também espelha os dados no MazeMapper para telemetria.
// ──────────────────────────────────────────────────────────────────────────────

void Navigation::storeWalls() {
    sensors_.update();

    bool wall_front = sensors_.hasWallFront();
    bool wall_left  = sensors_.hasWallLeft();
    bool wall_right = sensors_.hasWallRight();

    Direction abs_front = pose_.heading;
    Direction abs_left  = relLeft(pose_.heading);
    Direction abs_right = relRight(pose_.heading);

    ff_.setWall(pose_.x, pose_.y, abs_front,
                wall_front ? WALL_PRESENT : WALL_ABSENT);
    ff_.setWall(pose_.x, pose_.y, abs_left,
                wall_left  ? WALL_PRESENT : WALL_ABSENT);
    ff_.setWall(pose_.x, pose_.y, abs_right,
                wall_right ? WALL_PRESENT : WALL_ABSENT);

    ff_.setVisited(pose_.x, pose_.y);

    // Sincroniza com MazeMapper (para telemetria MQTT)
    bool n = (ff_.getWall(pose_.x, pose_.y, DIR_NORTH) == WALL_PRESENT);
    bool s = (ff_.getWall(pose_.x, pose_.y, DIR_SOUTH) == WALL_PRESENT);
    bool e = (ff_.getWall(pose_.x, pose_.y, DIR_EAST)  == WALL_PRESENT);
    bool w = (ff_.getWall(pose_.x, pose_.y, DIR_WEST)  == WALL_PRESENT);
    mapper_.updateCell(pose_.x, pose_.y, n, s, e, w);
}

// ──────────────────────────────────────────────────────────────────────────────
// Calcula o número mínimo de giros entre o heading atual e o desejado,
// depois executa o movimento físico e atualiza pose_.heading.
// ──────────────────────────────────────────────────────────────────────────────

void Navigation::executeTurn(Direction desired) {
    int diff = ((int)desired - (int)pose_.heading + 4) % 4;
    switch (diff) {
        case 0: break;                                  // já na direção certa
        case 1: rotacao_.girar(90); break;              // 90° direita
        case 2: rotacao_.girar(180); break;             // 180°
        case 3: rotacao_.girar(-90); break;             // 90° esquerda
    }
    pose_.heading = desired;
}

void Navigation::advancePosition() {
    pose_.x = (uint8_t)(pose_.x + FloodFill::DX[pose_.heading]);
    pose_.y = (uint8_t)(pose_.y + FloodFill::DY[pose_.heading]);
}

// ──────────────────────────────────────────────────────────────────────────────

void Navigation::stepTowardTarget() {
    // Recalcula sempre — garante que novas paredes sejam consideradas
    if (state_ == NAV_RETURN_HOME) {
        ff_.recompute(NAV_HOME_X, NAV_HOME_Y);
    } else {
        ff_.recomputeMulti(goalX_, goalY_, goalCount_);
    }

    Direction best = ff_.findBestDirection(pose_.x, pose_.y);
    if (best == DIR_NONE) {
        printf("[NAV] ERRO: sem caminho em (%d,%d) peso=%d\r\n",
               pose_.x, pose_.y, ff_.getWeight(pose_.x, pose_.y));
        return;
    }

    executeTurn(best);
    movFrente_.moverFrente();
    advancePosition();

    printf("[NAV] (%d,%d) heading=%s peso=%d\r\n",
           pose_.x, pose_.y, FloodFill::dirToStr(pose_.heading),
           ff_.getWeight(pose_.x, pose_.y));
}

// ──────────────────────────────────────────────────────────────────────────────
// Loop principal — chamar repetidamente no main loop.
// ──────────────────────────────────────────────────────────────────────────────

void Navigation::update() {
    if (state_ == NAV_DONE) return;

    // 1. Registra paredes da célula atual antes de mover
    storeWalls();

    // 2. Move um passo em direção ao alvo atual
    stepTowardTarget();

    // 3. Verifica se atingiu o alvo e transiciona de estado
    if (state_ == NAV_EXPLORE) {
        if (isAtGoal(pose_.x, pose_.y)) {
            printf("[NAV] Centro atingido! Retornando a origem.\r\n");
            state_ = NAV_RETURN_HOME;
            // Recalcula flood fill para o caminho de volta
            ff_.recompute(NAV_HOME_X, NAV_HOME_Y);
        }
    } else if (state_ == NAV_RETURN_HOME) {
        if (pose_.x == NAV_HOME_X && pose_.y == NAV_HOME_Y) {
            uint8_t n = ff_.size();
            printf("[NAV] Origem atingida! Exploracao concluida. Celulas: %d/%d\r\n",
                   ff_.visitedCount(), (int)n * (int)n);
            state_ = NAV_DONE;
            movFrente_.parar();
        }
    }
}

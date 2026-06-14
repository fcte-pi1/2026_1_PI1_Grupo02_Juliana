#include "Navigation.h"
#include <stdio.h>

Navigation::Navigation(FloodFill& ff, MazeMapper& mapper,
                       SensorManager& sensors, MotorController& motors)
    : ff_(ff), mapper_(mapper), sensors_(sensors), motors_(motors),
      state_(NAV_EXPLORE) {}

// ──────────────────────────────────────────────────────────────────────────────

void Navigation::init() {
    pose_ = {NAV_HOME_X, NAV_HOME_Y, DIR_NORTH};
    state_ = NAV_EXPLORE;

    ff_.init();
    ff_.recompute(NAV_TARGET_X, NAV_TARGET_Y);

    printf("[NAV] Inicio em (%d,%d) heading=%s alvo=(%d,%d)\r\n",
           pose_.x, pose_.y, FloodFill::dirToStr(pose_.heading),
           NAV_TARGET_X, NAV_TARGET_Y);
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
        case 1: motors_.turnRight(); break;             // 90° direita
        case 2: motors_.turnAround(); break;            // 180°
        case 3: motors_.turnLeft();  break;             // 90° esquerda
    }
    pose_.heading = desired;
}

void Navigation::advancePosition() {
    pose_.x = (uint8_t)(pose_.x + FloodFill::DX[pose_.heading]);
    pose_.y = (uint8_t)(pose_.y + FloodFill::DY[pose_.heading]);
}

// ──────────────────────────────────────────────────────────────────────────────

void Navigation::stepTowardTarget() {
    uint8_t tx = (state_ == NAV_RETURN_HOME) ? NAV_HOME_X : NAV_TARGET_X;
    uint8_t ty = (state_ == NAV_RETURN_HOME) ? NAV_HOME_Y : NAV_TARGET_Y;

    // Recalcula sempre — garante que novas paredes sejam consideradas
    ff_.recompute(tx, ty);

    Direction best = ff_.findBestDirection(pose_.x, pose_.y);
    if (best == DIR_NONE) {
        printf("[NAV] ERRO: sem caminho em (%d,%d) peso=%d\r\n",
               pose_.x, pose_.y, ff_.getWeight(pose_.x, pose_.y));
        return;
    }

    executeTurn(best);
    motors_.moveForward();
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
        if (pose_.x == NAV_TARGET_X && pose_.y == NAV_TARGET_Y) {
            printf("[NAV] Centro atingido! Retornando a origem.\r\n");
            state_ = NAV_RETURN_HOME;
            // Recalcula flood fill para o caminho de volta
            ff_.recompute(NAV_HOME_X, NAV_HOME_Y);
        }
    } else if (state_ == NAV_RETURN_HOME) {
        if (pose_.x == NAV_HOME_X && pose_.y == NAV_HOME_Y) {
            printf("[NAV] Origem atingida! Exploracao concluida. Celulas: %d/256\r\n",
                   ff_.visitedCount());
            state_ = NAV_DONE;
            motors_.stop();
        }
    }
}

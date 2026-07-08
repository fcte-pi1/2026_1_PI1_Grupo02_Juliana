#include "MazeMapper.h"
#include <stdio.h>

MazeMapper::MazeMapper(FloodFill& ff) : ff_(ff) {}

// ──────────────────────────────────────────────────────────────────────────────
// Atualiza paredes no FloodFill delegando para ff_.setWall().
//
// Política de atualização em re-visitas:
//   • Se a parede ainda é UNKNOWN → atualiza com o valor do sensor.
//   • Se a parede já é PRESENT   → mantém PRESENT (sensor pode falhar).
//   • Se a parede já é ABSENT    → atualiza apenas se o sensor detectou PRESENT
//     (um falso negativo anterior pode ser corrigido; falso positivo não, pois
//     paredes físicas não desaparecem).
// ──────────────────────────────────────────────────────────────────────────────

static void updateWallIfNeeded(FloodFill& ff, uint8_t x, uint8_t y,
                               Direction dir, bool sensor_says_present) {
    WallState current = ff.getWall(x, y, dir);
    WallState new_state = sensor_says_present ? WALL_PRESENT : WALL_ABSENT;

    if (current == WALL_UNKNOWN) {
        ff.setWall(x, y, dir, new_state);
    } else if (current == WALL_ABSENT && sensor_says_present) {
        // Corrige falso negativo anterior
        ff.setWall(x, y, dir, WALL_PRESENT);
    }
    // Se current == WALL_PRESENT: nunca remove
}

void MazeMapper::updateCell(uint8_t x, uint8_t y,
                            bool wall_n, bool wall_s, bool wall_e, bool wall_w) {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return;

    updateWallIfNeeded(ff_, x, y, DIR_NORTH, wall_n);
    updateWallIfNeeded(ff_, x, y, DIR_SOUTH, wall_s);
    updateWallIfNeeded(ff_, x, y, DIR_EAST,  wall_e);
    updateWallIfNeeded(ff_, x, y, DIR_WEST,  wall_w);

    ff_.setVisited(x, y);
}

float MazeMapper::explorationPercentage() const {
    return (ff_.visitedCount() * 100.0f) / (FF_MAZE_SIZE * FF_MAZE_SIZE);
}

// ──────────────────────────────────────────────────────────────────────────────
// Payload JSON no formato estabelecido pelo projeto (compatível com telemetria).
//
// Exemplo de saída:
// {
//   "ts": "...", "run_id": "micromouse_001",
//   "pose": {"x":3,"y":4,"heading":"N"},
//   "maze_delta": [{"x":3,"y":4,"walls":{"n":true,"s":false,"e":false,"w":true},"weight":12}],
//   "explored_pct": 14.45,
//   "speed": 0.34, "battery": 78, "voltage": 7.42
// }
// ──────────────────────────────────────────────────────────────────────────────

void MazeMapper::generatePayload(char*       buffer,
                                 size_t      buffer_size,
                                 uint8_t     x,
                                 uint8_t     y,
                                 const char* heading_str,
                                 const char* run_id,
                                 float       speed,
                                 int         battery,
                                 float       voltage) const {
    const MazeCell& c = ff_.cell(x, y);
    float pct = explorationPercentage();

    snprintf(buffer, buffer_size,
        "{\"ts\":\"2026-01-01T00:00:00Z\","
        "\"run_id\":\"%s\","
        "\"pose\":{\"x\":%d,\"y\":%d,\"heading\":\"%s\"},"
        "\"maze_delta\":[{"
            "\"x\":%d,\"y\":%d,"
            "\"walls\":{\"n\":%s,\"s\":%s,\"e\":%s,\"w\":%s},"
            "\"weight\":%d"
        "}],"
        "\"explored_pct\":%.2f,"
        "\"speed\":%.2f,\"battery\":%d,\"voltage\":%.2f}",
        run_id,
        (int)x, (int)y, heading_str,
        (int)x, (int)y,
        (c.wall[DIR_NORTH] == WALL_PRESENT) ? "true" : "false",
        (c.wall[DIR_SOUTH] == WALL_PRESENT) ? "true" : "false",
        (c.wall[DIR_EAST]  == WALL_PRESENT) ? "true" : "false",
        (c.wall[DIR_WEST]  == WALL_PRESENT) ? "true" : "false",
        (int)c.weight,
        pct,
        speed, battery, voltage);
}

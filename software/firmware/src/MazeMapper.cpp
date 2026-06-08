#include "MazeMapper.h"
#include <stdio.h>
#include <math.h>

Cell maze[MAZE_SIZE][MAZE_SIZE];
int celulas_exploradas = 0;

void atualizar_mapa(int x, int y, bool sensor_n, bool sensor_s, bool sensor_e, bool sensor_w) {
    if (x < 0 || x >= MAZE_SIZE || y < 0 || y >= MAZE_SIZE) return;

    if (maze[x][y].visited) {
        return; 
    }

    maze[x][y].n = sensor_n;
    maze[x][y].s = sensor_s;
    maze[x][y].e = sensor_e;
    maze[x][y].w = sensor_w;

    maze[x][y].visited = true;
    celulas_exploradas++;
}

float calcular_porcentagem_explorada() {
    return ((float)celulas_exploradas / TOTAL_CELLS) * 100.0f;
}

void gerar_payload_telemetria(char* buffer, size_t buffer_size, int x, int y, const char* run_id) {
    float percentual = calcular_porcentagem_explorada();
    
    // Monta o JSON manualmente (sem ArduinoJson)
    snprintf(buffer, buffer_size,
        "{\"ts\":\"2026-05-24T00:00:00Z\",\"run_id\":\"%s\","
        "\"pose\":{\"x\":%d,\"y\":%d,\"heading\":\"N\"},"
        "\"maze_delta\":[{"
        "\"x\":%d,\"y\":%d,"
        "\"walls\":{\"n\":%s,\"s\":%s,\"e\":%s,\"w\":%s}"
        "}],"
        "\"explored_percentage\":%.2f,"
        "\"speed\":0.34,\"battery\":78,\"voltage\":7.42}",
        run_id, x, y, x, y,
        maze[x][y].n ? "true" : "false",
        maze[x][y].s ? "true" : "false",
        maze[x][y].e ? "true" : "false",
        maze[x][y].w ? "true" : "false",
        percentual);
}
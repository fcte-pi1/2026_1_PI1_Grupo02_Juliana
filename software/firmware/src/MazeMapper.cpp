#include "MazeMapper.h"


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
    return ((float)celulas_exploradas / TOTAL_CELLS) * 100.0;
}
String gerar_payload_telemetria(int x, int y, const char* run_id) {
    JsonDocument doc;
    
    doc["ts"] = "2026-05-24T00:00:00Z"; 
    doc["run_id"] = run_id;
    
    // Sintaxe nova (V7): Muito mais direto e limpo!
    doc["pose"]["x"] = x;
    doc["pose"]["y"] = y;
    doc["pose"]["heading"] = "N"; 
    
    // Adiciona um objeto dentro do array maze_delta
    JsonObject delta_cell = doc["maze_delta"].add<JsonObject>();
    delta_cell["x"] = x;
    delta_cell["y"] = y;
    
    delta_cell["walls"]["n"] = maze[x][y].n;
    delta_cell["walls"]["s"] = maze[x][y].s;
    delta_cell["walls"]["e"] = maze[x][y].e;
    delta_cell["walls"]["w"] = maze[x][y].w;

    doc["explored_percentage"] = calcular_porcentagem_explorada();
    doc["speed"] = 0.34;   
    doc["battery"] = 78;   
    doc["voltage"] = 7.42;

    String saida;
    serializeJson(doc, saida);
    return saida;
}
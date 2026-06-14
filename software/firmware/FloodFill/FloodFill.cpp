#include "FloodFill.h"
#include <string.h>

// Deslocamentos cardinais: N=0, E=1, S=2, W=3
// Coordenadas: (0,0) = canto inferior esquerdo, Y cresce para Norte
const int8_t FloodFill::DX[4] = {  0, +1,  0, -1 };
const int8_t FloodFill::DY[4] = { +1,  0, -1,  0 };

// ──────────────────────────────────────────────────────────────────────────────

FloodFill::FloodFill() {
    init();
}

void FloodFill::init() {
    for (int x = 0; x < FF_MAZE_SIZE; x++) {
        for (int y = 0; y < FF_MAZE_SIZE; y++) {
            cells_[x][y].weight  = FF_WEIGHT_INF;
            cells_[x][y].visited = false;
            for (int d = 0; d < 4; d++)
                cells_[x][y].wall[d] = WALL_UNKNOWN;
        }
    }
    // Borda sul (y=0): parede ao sul de todas as células da linha 0
    // Borda norte (y=15): parede ao norte de todas as células da linha 15
    // Borda oeste (x=0): parede ao oeste
    // Borda leste (x=15): parede ao leste
    for (int i = 0; i < FF_MAZE_SIZE; i++) {
        cells_[i][0].wall[DIR_SOUTH]               = WALL_PRESENT;
        cells_[i][FF_MAZE_SIZE - 1].wall[DIR_NORTH] = WALL_PRESENT;
        cells_[0][i].wall[DIR_WEST]                = WALL_PRESENT;
        cells_[FF_MAZE_SIZE - 1][i].wall[DIR_EAST] = WALL_PRESENT;
    }
}

// ──────────────────────────────────────────────────────────────────────────────

void FloodFill::setWall(uint8_t x, uint8_t y, Direction dir, WallState state) {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return;
    cells_[x][y].wall[dir] = state;

    // Propaga para a célula adjacente (face oposta) para manter consistência
    int nx = x + DX[dir];
    int ny = y + DY[dir];
    if (nx >= 0 && nx < FF_MAZE_SIZE && ny >= 0 && ny < FF_MAZE_SIZE) {
        Direction opp = static_cast<Direction>((dir + 2) % 4);
        cells_[nx][ny].wall[opp] = state;
    }
}

WallState FloodFill::getWall(uint8_t x, uint8_t y, Direction dir) const {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return WALL_PRESENT;
    return cells_[x][y].wall[dir];
}

uint8_t FloodFill::getWeight(uint8_t x, uint8_t y) const {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return FF_WEIGHT_INF;
    return cells_[x][y].weight;
}

void FloodFill::setVisited(uint8_t x, uint8_t y) {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return;
    cells_[x][y].visited = true;
}

bool FloodFill::isVisited(uint8_t x, uint8_t y) const {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return false;
    return cells_[x][y].visited;
}

int FloodFill::visitedCount() const {
    int count = 0;
    for (int x = 0; x < FF_MAZE_SIZE; x++)
        for (int y = 0; y < FF_MAZE_SIZE; y++)
            if (cells_[x][y].visited) count++;
    return count;
}

const MazeCell& FloodFill::cell(uint8_t x, uint8_t y) const {
    // Sem bounds check intencionalmente — chamador deve garantir coordenadas válidas
    return cells_[x][y];
}

// ──────────────────────────────────────────────────────────────────────────────
// BFS padrão — O(N) onde N = número de células (256 para labirinto 16×16)
//
// Paredes UNKNOWN são tratadas como ABSENT: permite ao flood fill propagar por
// regiões ainda não exploradas, incentivando o robô a ir explorar.
// Após normalização (normalize_maze), UNKNOWN → PRESENT e o mapa fica final.
// ──────────────────────────────────────────────────────────────────────────────

void FloodFill::recompute(uint8_t tx, uint8_t ty) {
    if (tx >= FF_MAZE_SIZE || ty >= FF_MAZE_SIZE) return;

    // Reinicia todos os pesos
    for (int x = 0; x < FF_MAZE_SIZE; x++)
        for (int y = 0; y < FF_MAZE_SIZE; y++)
            cells_[x][y].weight = FF_WEIGHT_INF;

    // BFS da célula alvo
    int head = 0, tail = 0;
    cells_[tx][ty].weight = 0;
    bfs_queue_[tail++] = {tx, ty};

    while (head < tail) {
        QEntry cur = bfs_queue_[head++];
        uint8_t w = cells_[cur.x][cur.y].weight;

        for (int d = 0; d < 4; d++) {
            // Parede PRESENT bloqueia; UNKNOWN e ABSENT permitem passagem
            if (cells_[cur.x][cur.y].wall[d] == WALL_PRESENT) continue;

            int nx = cur.x + DX[d];
            int ny = cur.y + DY[d];
            if (nx < 0 || nx >= FF_MAZE_SIZE || ny < 0 || ny >= FF_MAZE_SIZE) continue;

            // Só enfileira se encontrarmos um caminho mais curto
            if (cells_[nx][ny].weight > (uint8_t)(w + 1)) {
                cells_[nx][ny].weight = w + 1;
                bfs_queue_[tail++] = {(uint8_t)nx, (uint8_t)ny};
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Retorna a direção do vizinho acessível com MENOR peso.
// Corrige o bug do MMS onde o último vizinho válido sobrescrevia o melhor.
// ──────────────────────────────────────────────────────────────────────────────

Direction FloodFill::findBestDirection(uint8_t x, uint8_t y) const {
    if (x >= FF_MAZE_SIZE || y >= FF_MAZE_SIZE) return DIR_NONE;

    uint8_t   best_weight = cells_[x][y].weight;  // começa com o peso local
    Direction best_dir    = DIR_NONE;

    for (int d = 0; d < 4; d++) {
        if (cells_[x][y].wall[d] == WALL_PRESENT) continue;

        int nx = x + DX[d];
        int ny = y + DY[d];
        if (nx < 0 || nx >= FF_MAZE_SIZE || ny < 0 || ny >= FF_MAZE_SIZE) continue;

        // Atualiza apenas se o vizinho tiver peso ESTRITAMENTE menor
        if (cells_[nx][ny].weight < best_weight) {
            best_weight = cells_[nx][ny].weight;
            best_dir    = static_cast<Direction>(d);
        }
    }
    return best_dir;
}

// ──────────────────────────────────────────────────────────────────────────────

const char* FloodFill::dirToStr(Direction d) {
    static const char* names[] = {"N", "E", "S", "W", "?"};
    return (d <= DIR_NONE) ? names[d] : names[4];
}

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

void FloodFill::init(uint8_t size) {
    size_ = (size == 0 || size > FF_MAZE_SIZE_MAX) ? FF_MAZE_SIZE_MAX : size;

    for (int x = 0; x < size_; x++) {
        for (int y = 0; y < size_; y++) {
            cells_[x][y].weight  = FF_WEIGHT_INF;
            cells_[x][y].visited = false;
            for (int d = 0; d < 4; d++)
                cells_[x][y].wall[d] = WALL_UNKNOWN;
        }
    }
    // Borda sul (y=0): parede ao sul de todas as células da linha 0
    // Borda norte (y=size-1): parede ao norte de todas as células da linha size-1
    // Borda oeste (x=0): parede ao oeste
    // Borda leste (x=size-1): parede ao leste
    for (int i = 0; i < size_; i++) {
        cells_[i][0].wall[DIR_SOUTH]           = WALL_PRESENT;
        cells_[i][size_ - 1].wall[DIR_NORTH]   = WALL_PRESENT;
        cells_[0][i].wall[DIR_WEST]            = WALL_PRESENT;
        cells_[size_ - 1][i].wall[DIR_EAST]    = WALL_PRESENT;
    }
}

uint8_t FloodFill::size() const { return size_; }

// ──────────────────────────────────────────────────────────────────────────────

void FloodFill::setWall(uint8_t x, uint8_t y, Direction dir, WallState state) {
    if (x >= size_ || y >= size_) return;
    cells_[x][y].wall[dir] = state;

    // Propaga para a célula adjacente (face oposta) para manter consistência
    int nx = x + DX[dir];
    int ny = y + DY[dir];
    if (nx >= 0 && nx < size_ && ny >= 0 && ny < size_) {
        Direction opp = static_cast<Direction>((dir + 2) % 4);
        cells_[nx][ny].wall[opp] = state;
    }
}

WallState FloodFill::getWall(uint8_t x, uint8_t y, Direction dir) const {
    if (x >= size_ || y >= size_) return WALL_PRESENT;
    return cells_[x][y].wall[dir];
}

uint8_t FloodFill::getWeight(uint8_t x, uint8_t y) const {
    if (x >= size_ || y >= size_) return FF_WEIGHT_INF;
    return cells_[x][y].weight;
}

void FloodFill::setVisited(uint8_t x, uint8_t y) {
    if (x >= size_ || y >= size_) return;
    cells_[x][y].visited = true;
}

bool FloodFill::isVisited(uint8_t x, uint8_t y) const {
    if (x >= size_ || y >= size_) return false;
    return cells_[x][y].visited;
}

int FloodFill::visitedCount() const {
    int count = 0;
    for (int x = 0; x < size_; x++)
        for (int y = 0; y < size_; y++)
            if (cells_[x][y].visited) count++;
    return count;
}

const MazeCell& FloodFill::cell(uint8_t x, uint8_t y) const {
    // Sem bounds check intencionalmente — chamador deve garantir coordenadas válidas
    return cells_[x][y];
}

// ──────────────────────────────────────────────────────────────────────────────
// BFS multi-origem — O(N) onde N = número de células (size_²)
//
// Paredes UNKNOWN são tratadas como ABSENT: permite ao flood fill propagar por
// regiões ainda não exploradas, incentivando o robô a ir explorar.
// Após normalização (normalize_maze), UNKNOWN → PRESENT e o mapa fica final.
//
// Semear a fila com várias células (peso 0 cada) resolve o caso do alvo ser uma
// ÁREA (ex.: bloco central 2x2), não só um ponto: o peso de cada célula passa a
// refletir a distância até o alvo mais próximo dentre os informados.
// ──────────────────────────────────────────────────────────────────────────────

void FloodFill::recomputeMulti(const uint8_t* txs, const uint8_t* tys, uint8_t count) {
    for (int x = 0; x < size_; x++)
        for (int y = 0; y < size_; y++)
            cells_[x][y].weight = FF_WEIGHT_INF;

    int head = 0, tail = 0;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t tx = txs[i], ty = tys[i];
        if (tx >= size_ || ty >= size_) continue;
        if (cells_[tx][ty].weight == 0) continue; // já semeada
        cells_[tx][ty].weight = 0;
        bfs_queue_[tail++] = {tx, ty};
    }

    while (head < tail) {
        QEntry cur = bfs_queue_[head++];
        uint8_t w = cells_[cur.x][cur.y].weight;

        for (int d = 0; d < 4; d++) {
            // Parede PRESENT bloqueia; UNKNOWN e ABSENT permitem passagem
            if (cells_[cur.x][cur.y].wall[d] == WALL_PRESENT) continue;

            int nx = cur.x + DX[d];
            int ny = cur.y + DY[d];
            if (nx < 0 || nx >= size_ || ny < 0 || ny >= size_) continue;

            // Só enfileira se encontrarmos um caminho mais curto
            if (cells_[nx][ny].weight > (uint8_t)(w + 1)) {
                cells_[nx][ny].weight = w + 1;
                bfs_queue_[tail++] = {(uint8_t)nx, (uint8_t)ny};
            }
        }
    }
}

void FloodFill::recompute(uint8_t tx, uint8_t ty) {
    recomputeMulti(&tx, &ty, 1);
}

// ──────────────────────────────────────────────────────────────────────────────
// Retorna a direção do vizinho acessível com MENOR peso.
// Corrige o bug do MMS onde o último vizinho válido sobrescrevia o melhor.
// ──────────────────────────────────────────────────────────────────────────────

Direction FloodFill::findBestDirection(uint8_t x, uint8_t y) const {
    if (x >= size_ || y >= size_) return DIR_NONE;

    uint8_t   best_weight = cells_[x][y].weight;  // começa com o peso local
    Direction best_dir    = DIR_NONE;

    for (int d = 0; d < 4; d++) {
        if (cells_[x][y].wall[d] == WALL_PRESENT) continue;

        int nx = x + DX[d];
        int ny = y + DY[d];
        if (nx < 0 || nx >= size_ || ny < 0 || ny >= size_) continue;

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

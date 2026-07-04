#pragma once
#include <stdint.h>
#include <stdbool.h>

// ──────────────────────────────────────────────────────────────────────────────
// FloodFill — única fonte de verdade para o mapa do labirinto
//
// Armazena paredes, pesos (distâncias BFS) e status de visita para cada célula.
// Não tem nenhuma dependência de hardware — pode ser testado em PC ou simulador.
//
// O tamanho do labirinto é definido em runtime via init(size) (ex.: 4, 8 ou 16),
// permitindo reconfigurar o mesmo robô para labirintos diferentes sem recompilar.
// FF_MAZE_SIZE_MAX é só a capacidade máxima dos arrays estáticos.
// ──────────────────────────────────────────────────────────────────────────────

#define FF_MAZE_SIZE_MAX 16
#define FF_WEIGHT_INF    255
#define FF_MAX_GOALS     4   // bloco central 2x2 no máximo

// Estado de cada face de parede
enum WallState : uint8_t {
    WALL_UNKNOWN = 0,  // ainda não detectada pelo sensor
    WALL_ABSENT,       // sensor confirmou ausência
    WALL_PRESENT,      // sensor confirmou presença
};

// Direção cardinal — indexa wall[] e os arrays DX/DY
enum Direction : uint8_t {
    DIR_NORTH = 0,
    DIR_EAST  = 1,
    DIR_SOUTH = 2,
    DIR_WEST  = 3,
    DIR_NONE  = 4,     // retorno quando não há caminho
};

// Célula do labirinto — tudo em 7 bytes, alinhável em 8
struct MazeCell {
    uint8_t   weight;     // distância BFS até o alvo (FF_WEIGHT_INF = não calculado)
    WallState wall[4];    // indexed by Direction: [N, E, S, W]
    bool      visited;    // robô já esteve nesta célula
};

// ──────────────────────────────────────────────────────────────────────────────
class FloodFill {
public:
    FloodFill();

    // Reinicia o mapa para um labirinto size x size (size <= FF_MAZE_SIZE_MAX).
    // Pesos = INF, paredes = UNKNOWN, visited = false.
    // Paredes de borda são marcadas como PRESENT automaticamente.
    void init(uint8_t size = FF_MAZE_SIZE_MAX);

    // Tamanho atual configurado (lado do labirinto quadrado).
    uint8_t size() const;

    // Recalcula todos os pesos via BFS a partir de uma única célula alvo (tx, ty).
    // Uso típico: caminho de volta à origem.
    void recompute(uint8_t tx, uint8_t ty);

    // Recalcula pesos via BFS multi-origem a partir de várias células alvo simultâneas
    // (ex.: bloco central 2x2). Cada célula listada começa com peso 0 — o BFS propaga
    // a partir de todas ao mesmo tempo, então o peso de cada célula reflete a distância
    // até o alvo MAIS PRÓXIMO, não a um alvo fixo específico.
    // Paredes UNKNOWN são tratadas como ABSENT durante a exploração.
    void recomputeMulti(const uint8_t* txs, const uint8_t* tys, uint8_t count);

    // Define o estado de uma parede e propaga para a célula vizinha (consistência).
    void      setWall(uint8_t x, uint8_t y, Direction dir, WallState state);
    WallState getWall(uint8_t x, uint8_t y, Direction dir) const;

    uint8_t   getWeight(uint8_t x, uint8_t y) const;

    void setVisited(uint8_t x, uint8_t y);
    bool isVisited(uint8_t x, uint8_t y)  const;
    int  visitedCount()                    const;

    // Retorna a direção do vizinho acessível com menor peso.
    // Retorna DIR_NONE se não houver caminho ou se já estiver no alvo.
    Direction findBestDirection(uint8_t x, uint8_t y) const;

    // Acesso read-only à célula bruta (para telemetria/debug).
    const MazeCell& cell(uint8_t x, uint8_t y) const;

    // Converte direção para string "N"/"E"/"S"/"W".
    static const char* dirToStr(Direction d);

    // Deslocamentos DX/DY por direção: índice = Direction enum
    // DX: {0, +1,  0, -1}  (N, E, S, W)
    // DY: {+1, 0, -1,  0}
    static const int8_t DX[4];
    static const int8_t DY[4];

private:
    uint8_t  size_;
    MazeCell cells_[FF_MAZE_SIZE_MAX][FF_MAZE_SIZE_MAX];

    // Fila estática para BFS (capacidade máxima = FF_MAZE_SIZE_MAX²)
    struct QEntry { uint8_t x, y; };
    QEntry bfs_queue_[FF_MAZE_SIZE_MAX * FF_MAZE_SIZE_MAX];
};

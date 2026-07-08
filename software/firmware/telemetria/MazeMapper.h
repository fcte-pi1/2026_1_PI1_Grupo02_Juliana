#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../FloodFill/FloodFill.h"

// ──────────────────────────────────────────────────────────────────────────────
// MazeMapper — camada de telemetria sobre o FloodFill
//
// Não armazena paredes nem pesos próprios: esses dados vivem exclusivamente no
// FloodFill, que é a única fonte de verdade. O MazeMapper apenas lê o FloodFill
// para formatar payloads JSON e calcular métricas de exploração.
// ──────────────────────────────────────────────────────────────────────────────

class MazeMapper {
public:
    explicit MazeMapper(FloodFill& ff);

    // Atualiza o FloodFill com dados de sensor de uma célula.
    // Se a célula já foi visitada, paredes só são adicionadas (nunca removidas)
    // — sensores podem ter falso negativo, mas falso positivo é descartado.
    void updateCell(uint8_t x, uint8_t y,
                    bool wall_n, bool wall_s, bool wall_e, bool wall_w);

    // Porcentagem de células visitadas (0.0 .. 100.0)
    float explorationPercentage() const;

    // Gera payload JSON de telemetria compatível com o formato existente do projeto.
    // buffer deve ter ao menos 512 bytes.
    // heading_str: "N", "E", "S" ou "W"
    void generatePayload(char*       buffer,
                         size_t      buffer_size,
                         uint8_t     x,
                         uint8_t     y,
                         const char* heading_str,
                         const char* run_id,
                         float       speed,
                         int         battery,
                         float       voltage) const;

private:
    FloodFill& ff_;
};

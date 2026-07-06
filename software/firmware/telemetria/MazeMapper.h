#ifndef MAZE_MAPPER_H
#define MAZE_MAPPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// config do labrinto
const int MAZE_SIZE = 16;
const int TOTAL_CELLS = MAZE_SIZE * MAZE_SIZE;

// struct da celula
struct Cell {
    bool n = false;
    bool s = false;
    bool e = false;
    bool w = false;
    bool visited = false;
};

// variaveis globais
extern Cell maze[MAZE_SIZE][MAZE_SIZE];
extern int celulas_exploradas;

// funções 
void atualizar_mapa(int x, int y, bool sensor_n, bool sensor_s, bool sensor_e, bool sensor_w);
float calcular_porcentagem_explorada();
void gerar_payload_telemetria(char* buffer, size_t buffer_size, int x, int y, const char* run_id);

#endif
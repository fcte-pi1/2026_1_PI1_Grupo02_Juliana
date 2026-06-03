"""Helpers stateless de manipulação do mapa do labirinto.

`maze` é um dict esparso: chave `"x,y"` → flags de parede `{n,s,e,w}`.
Só células descobertas aparecem. Funções puras — não tocam o banco.
"""
from __future__ import annotations

from runs.schemas import MazeCell


def cell_key(x: int, y: int) -> str:
    return f"{x},{y}"


def apply_delta(maze: dict, delta: list[MazeCell]) -> dict:
    """Aplica as células do delta sobre o maze, retornando o dict atualizado.

    Mutação in-place + retorno (conveniência). Sobrescreve a célula inteira —
    o firmware envia o estado consolidado da célula, não diffs de parede.
    """
    for cell in delta:
        maze[cell_key(cell.x, cell.y)] = cell.walls.model_dump()
    return maze

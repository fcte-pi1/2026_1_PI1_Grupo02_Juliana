"""Schemas pydantic dos pacotes MQTT vindos do robô.

Validação na borda: o subscriber valida o JSON cru aqui antes de chamar os
use cases. Contrato definido em `docs/.../06_projetoconceitual.tex` (proposta,
a ratificar com firmware).
"""
from __future__ import annotations

from datetime import datetime
from typing import Literal

from pydantic import BaseModel, Field

Heading = Literal["N", "S", "E", "W"]


class WallFlags(BaseModel):
    n: bool = False
    s: bool = False
    e: bool = False
    w: bool = False


class MazeCell(BaseModel):
    x: int
    y: int
    walls: WallFlags


class Pose(BaseModel):
    x: int
    y: int
    heading: Heading


class TelemetriaPayload(BaseModel):
    ts: datetime
    run_id: str
    pose: Pose
    maze_delta: list[MazeCell] = Field(default_factory=list)
    speed: float | None = None
    battery: float | None = None
    voltage: float | None = None


class EventoPayload(BaseModel):
    ts: datetime
    run_id: str
    type: Literal["inicio", "colisao", "reparo", "desafio_cumprido"]
    detail: str = ""

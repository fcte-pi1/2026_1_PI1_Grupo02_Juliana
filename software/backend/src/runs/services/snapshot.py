"""Monta o snapshot que vai pro dashboard (REST inicial + SSE live).

Combina o estado persistido da `Tentativa` (maze, pose, status, métricas) com
valores instantâneos do último pacote (speed/battery/voltage), quando houver.
"""
from __future__ import annotations

from runs.models import Tentativa


def build_snapshot(
    tentativa: Tentativa,
    *,
    speed: float | None = None,
    battery: float | None = None,
    voltage: float | None = None,
    ts=None,
) -> dict:
    return {
        "tentativa_id": str(tentativa.id),
        "dimensao": tentativa.labirinto.dimensao,
        "status": tentativa.status,
        "pose": tentativa.pose or None,
        "maze": tentativa.maze,
        "explored": len(tentativa.maze),
        "velocidade_media": tentativa.velocidade_media,
        "consumo_bateria": tentativa.consumo_bateria,
        "speed": speed,
        "battery": battery if battery is not None else tentativa.consumo_bateria,
        "voltage": voltage,
        "ts": ts,
    }

"""Simulação de uma corrida — faz o papel do firmware enquanto não há hardware.

Percorre um caminho dentro do grid e publica pacotes `telemetria` (com `maze_delta`
incremental) + eventos no MQTT, exatamente como o robô faria. Reutilizado pela
management command `mqtt_simulate` e pela task Celery `simular_corrida` (acionada
pelo botão "Iniciar run" do dashboard).
"""
from __future__ import annotations

import json
import time
from collections.abc import Callable
from datetime import UTC, datetime

from integrations.mqtt.client import build_client, topic_evento, topic_telemetria
from runs.models import Tentativa

DIRS = ("n", "s", "e", "w")


def _direction(frm: tuple[int, int], to: tuple[int, int]) -> str:
    dx, dy = to[0] - frm[0], to[1] - frm[1]
    if dy < 0:
        return "n"
    if dy > 0:
        return "s"
    return "e" if dx > 0 else "w"


def _heading(direction: str) -> str:
    return {"n": "N", "s": "S", "e": "E", "w": "W"}[direction]


def _boustrophedon(n: int, max_steps: int) -> list[tuple[int, int]]:
    """Caminho em ziguezague cobrindo o grid, truncado no centro ou em max_steps."""
    path: list[tuple[int, int]] = []
    center = (n // 2, n // 2)
    for y in range(n):
        cols = range(n) if y % 2 == 0 else range(n - 1, -1, -1)
        for x in cols:
            path.append((x, y))
            if (x, y) == center or len(path) >= max_steps:
                return path
    return path


def _walls_for(cell: tuple[int, int], prev, nxt) -> dict:
    open_dirs = set()
    if prev is not None:
        open_dirs.add(_direction(cell, prev))
    if nxt is not None:
        open_dirs.add(_direction(cell, nxt))
    return {d: d not in open_dirs for d in DIRS}


def _publish_evento(client, mm_id: str, run_id: str, tipo: str) -> None:
    payload = {"ts": datetime.now(UTC).isoformat(), "run_id": run_id, "type": tipo, "detail": ""}
    client.publish(topic_evento(mm_id), json.dumps(payload), qos=1, retain=True)


def run_simulation(
    *,
    tentativa: Tentativa,
    hz: float = 2.0,
    steps: int = 48,
    should_stop: Callable[[], bool] | None = None,
    log: Callable[[str], None] | None = None,
) -> bool:
    """Publica a corrida simulada via MQTT. Retorna True se completou, False se parou.

    `should_stop` é consultado a cada passo — permite o botão "Parar run" abortar.
    """
    n = tentativa.labirinto.dimensao
    mm_id = str(tentativa.micromouse_id)
    run_id = str(tentativa.id)

    client = build_client(client_id=f"firmware-sim-{run_id[:8]}")
    client.loop_start()
    path = _boustrophedon(n, steps)
    _publish_evento(client, mm_id, run_id, "inicio")

    battery = 100.0
    completed = True
    for i, cell in enumerate(path):
        if should_stop and should_stop():
            completed = False
            break
        prev = path[i - 1] if i > 0 else None
        nxt = path[i + 1] if i < len(path) - 1 else None
        walls = _walls_for(cell, prev, nxt)
        heading = _heading(_direction(cell, nxt)) if nxt else "N"
        battery = max(0.0, battery - 0.4)
        payload = {
            "ts": datetime.now(UTC).isoformat(),
            "run_id": run_id,
            "pose": {"x": cell[0], "y": cell[1], "heading": heading},
            "maze_delta": [{"x": cell[0], "y": cell[1], "walls": walls}],
            "speed": round(0.25 + 0.1 * (i % 3), 3),
            "battery": round(battery, 1),
            "voltage": round(7.4 - (100 - battery) * 0.01, 2),
        }
        client.publish(topic_telemetria(mm_id), json.dumps(payload), qos=1)
        if log:
            log(f"célula {cell} heading={heading}")
        time.sleep(1.0 / hz)

    if completed:
        _publish_evento(client, mm_id, run_id, "desafio_cumprido")
    client.loop_stop()
    client.disconnect()
    return completed

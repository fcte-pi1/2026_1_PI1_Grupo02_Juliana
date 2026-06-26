"""Simulação de uma corrida — faz o papel do firmware enquanto não há hardware.

Percorre um caminho dentro do grid e publica pacotes `telemetria` (com `maze_delta`
incremental) + eventos no MQTT, exatamente como o robô faria. Reutilizado pela
management command `mqtt_simulate` e pela task Celery `simular_corrida` (acionada
pelo botão "Iniciar run" do dashboard).

Modos de exploração disponíveis:
  - "dfs"           : DFS com retrocesso, tende ao centro (padrão — mais realista)
  - "boustrophedon" : ziguezague clássico (legado)
"""
from __future__ import annotations

import json
import random
import time
from collections.abc import Callable
from datetime import UTC, datetime

from integrations.mqtt.client import build_client, topic_evento, topic_telemetria
from runs.models import Tentativa

DIRS = ("n", "s", "e", "w")
_REV = {"n": "s", "s": "n", "e": "w", "w": "e"}


def _direction(frm: tuple[int, int], to: tuple[int, int]) -> str:
    dx, dy = to[0] - frm[0], to[1] - frm[1]
    if dy < 0:
        return "n"
    if dy > 0:
        return "s"
    return "e" if dx > 0 else "w"


def _heading(direction: str) -> str:
    return {"n": "N", "s": "S", "e": "E", "w": "W"}[direction]


# ── Algoritmos de caminho ──────────────────────────────────────────────────────

def _boustrophedon(n: int, max_steps: int) -> tuple[list[tuple[int, int]], set]:
    """Caminho em ziguezague cobrindo o grid, truncado no centro ou em max_steps."""
    path: list[tuple[int, int]] = []
    center = (n // 2, n // 2)
    for y in range(n):
        cols = range(n) if y % 2 == 0 else range(n - 1, -1, -1)
        for x in cols:
            path.append((x, y))
            if (x, y) == center or len(path) >= max_steps:
                return path, _passages_from_path(path)
    return path, _passages_from_path(path)


def _passages_from_path(path: list[tuple[int, int]]) -> set[tuple[tuple[int, int], str]]:
    """Deriva passagens abertas a partir de um path simples (prev→cur→next)."""
    passages: set[tuple[tuple[int, int], str]] = set()
    for i, cell in enumerate(path):
        if i > 0:
            d = _direction(path[i - 1], cell)
            passages.add((path[i - 1], d))
            passages.add((cell, _REV[d]))
    return passages


def _dfs_trace(
    n: int, max_steps: int, rng: random.Random
) -> tuple[list[tuple[int, int]], set[tuple[tuple[int, int], str]]]:
    """DFS com retrocesso a partir de (0, n-1) em direção ao centro.

    Retorna a trajetória física (inclui movimentos de retrocesso) e o conjunto
    de passagens abertas conhecidas — usado para gerar paredes mais realistas.
    """
    start = (0, n - 1)
    goal = (n // 2, n // 2)
    visited: set[tuple[int, int]] = {start}
    trace: list[tuple[int, int]] = [start]
    open_passages: set[tuple[tuple[int, int], str]] = set()

    def _next_cells(cell: tuple[int, int]) -> list[tuple[str, tuple[int, int]]]:
        x, y = cell
        candidates = [("n", (x, y - 1)), ("s", (x, y + 1)), ("e", (x + 1, y)), ("w", (x - 1, y))]
        valid = [(d, nb) for d, nb in candidates if 0 <= nb[0] < n and 0 <= nb[1] < n and nb not in visited]
        if not valid:
            return []
        # 60 % das vezes prefere a célula mais próxima do objetivo
        if rng.random() < 0.60:
            valid.sort(key=lambda dn: abs(dn[1][0] - goal[0]) + abs(dn[1][1] - goal[1]))
        else:
            rng.shuffle(valid)
        return valid

    stack: list[tuple[tuple[int, int], list[tuple[str, tuple[int, int]]]]] = [
        (start, _next_cells(start))
    ]

    while stack and len(trace) < max_steps:
        cell, nbrs = stack[-1]
        if cell == goal:
            break
        if nbrs:
            direction, nxt = nbrs.pop(0)
            open_passages.add((cell, direction))
            open_passages.add((nxt, _REV[direction]))
            visited.add(nxt)
            trace.append(nxt)
            stack.append((nxt, _next_cells(nxt)))
        else:
            stack.pop()
            if stack:
                # Retrocede para o pai
                trace.append(stack[-1][0])

    return trace, open_passages


# ── Modelos de telemetria ──────────────────────────────────────────────────────

def _compute_speed(
    i: int,
    path: list[tuple[int, int]],
    base_speed: float,
    rng: random.Random,
) -> float:
    """Velocidade baseada no contexto: retas → acelerado, curvas → devagar."""
    if len(path) < 2 or i == 0 or i >= len(path) - 1:
        noise = rng.gauss(0, base_speed * 0.06)
        return round(max(0.05, base_speed * 0.65 + noise), 3)

    prev, cur, nxt = path[i - 1], path[i], path[i + 1]
    d_in = _direction(prev, cur)
    d_out = _direction(cur, nxt)

    if d_in == d_out:
        # Reta: aumenta com comprimento do segmento
        run_len = 1
        for j in range(i + 1, min(i + 6, len(path) - 1)):
            if _direction(path[j - 1], path[j]) == d_in:
                run_len += 1
            else:
                break
        speed = min(base_speed * (0.85 + 0.05 * run_len), base_speed * 1.45)
    elif _REV[d_in] == d_out:
        # Retrocesso: bem devagar
        speed = base_speed * 0.35
    else:
        # Curva de 90°
        speed = base_speed * 0.50

    noise = rng.gauss(0, base_speed * 0.07)
    return round(max(0.05, speed + noise), 3)


def _compute_battery(prev: float, speed: float, base_speed: float, rng: random.Random) -> float:
    """Consumo proporcional à velocidade, com ruído gaussiano."""
    drain = 0.25 + 0.20 * (speed / base_speed) + rng.gauss(0, 0.035)
    return round(max(0.0, prev - max(0.08, drain)), 1)


def _compute_voltage(battery_pct: float, rng: random.Random) -> float:
    """Curva de descarga LiPo 2S (6.0 V – 8.4 V), não linear."""
    ratio = (battery_pct / 100.0) ** 0.82
    voltage = 6.0 + 2.4 * ratio + rng.gauss(0, 0.015)
    return round(max(6.0, min(8.4, voltage)), 2)


# ── Construção do maze_delta ───────────────────────────────────────────────────

def _walls_for(
    cell: tuple[int, int],
    open_passages: set[tuple[tuple[int, int], str]],
) -> dict[str, bool]:
    """Paredes da célula baseadas nas passagens conhecidas (True = tem parede)."""
    return {d: (cell, d) not in open_passages for d in DIRS}


# ── MQTT helpers ──────────────────────────────────────────────────────────────

def _publish_evento(client, mm_id: str, run_id: str, tipo: str) -> None:
    payload = {"ts": datetime.now(UTC).isoformat(), "run_id": run_id, "type": tipo, "detail": ""}
    client.publish(topic_evento(mm_id), json.dumps(payload), qos=1, retain=True)


# ── Entry point ───────────────────────────────────────────────────────────────

def run_simulation(
    *,
    tentativa: Tentativa,
    hz: float = 2.0,
    steps: int = 48,
    mode: str = "dfs",
    base_speed: float = 0.30,
    battery_start: float = 100.0,
    should_stop: Callable[[], bool] | None = None,
    log: Callable[[str], None] | None = None,
) -> bool:
    """Publica a corrida simulada via MQTT. Retorna True se completou, False se parou.

    Parâmetros
    ----------
    mode          : "dfs" (padrão) ou "boustrophedon" (legado)
    base_speed    : velocidade base em m/s
    battery_start : bateria inicial (%)
    should_stop   : consultado a cada passo — permite o botão "Parar run" abortar
    """
    n = tentativa.labirinto.dimensao
    mm_id = str(tentativa.micromouse_id)
    run_id = str(tentativa.id)
    rng = random.Random()

    client = build_client(client_id=f"firmware-sim-{run_id[:8]}")
    client.loop_start()

    if mode == "boustrophedon":
        path, open_passages = _boustrophedon(n, steps)
    else:
        path, open_passages = _dfs_trace(n, steps, rng)

    _publish_evento(client, mm_id, run_id, "inicio")

    battery = battery_start
    completed = True

    # Frações de célula publicadas entre cada passo (posição contínua no labirinto).
    # O firmware real envia a posição física do encoder a ~10 Hz; aqui interpolamos.
    _INTER_FRACS = (0.33, 0.67)

    for i, cell in enumerate(path):
        if should_stop and should_stop():
            completed = False
            break

        nxt = path[i + 1] if i < len(path) - 1 else None
        heading = _heading(_direction(cell, nxt)) if nxt else "N"
        walls = _walls_for(cell, open_passages)
        speed = _compute_speed(i, path, base_speed, rng)
        battery = _compute_battery(battery, speed, base_speed, rng)
        voltage = _compute_voltage(battery, rng)

        # Pacote principal: chegada à célula + descoberta de paredes (maze_delta)
        payload = {
            "ts": datetime.now(UTC).isoformat(),
            "run_id": run_id,
            "pose": {"x": float(cell[0]), "y": float(cell[1]), "heading": heading},
            "maze_delta": [{"x": cell[0], "y": cell[1], "walls": walls}],
            "speed": speed,
            "battery": round(battery, 1),
            "voltage": voltage,
        }
        client.publish(topic_telemetria(mm_id), json.dumps(payload), qos=1)
        if log:
            log(f"célula {cell} heading={heading} speed={speed} m/s bat={battery}%")

        if nxt is None:
            time.sleep(1.0 / hz)
            continue

        # Pacotes intermediários: posição float entre célula atual e próxima.
        # maze_delta vazio — sem novas paredes descobertas durante o trânsito.
        interval = 1.0 / hz / (len(_INTER_FRACS) + 1)
        for frac in _INTER_FRACS:
            time.sleep(interval)
            inter_payload = {
                "ts": datetime.now(UTC).isoformat(),
                "run_id": run_id,
                "pose": {
                    "x": round(cell[0] + frac * (nxt[0] - cell[0]), 3),
                    "y": round(cell[1] + frac * (nxt[1] - cell[1]), 3),
                    "heading": heading,
                },
                "maze_delta": [],
                "speed": speed,
                "battery": round(battery, 1),
                "voltage": voltage,
            }
            client.publish(topic_telemetria(mm_id), json.dumps(inter_payload), qos=0)
        time.sleep(interval)

    if completed:
        _publish_evento(client, mm_id, run_id, "desafio_cumprido")
    client.loop_stop()
    client.disconnect()
    return completed

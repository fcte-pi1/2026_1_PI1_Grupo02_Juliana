"""Tests para os schemas Pydantic de runs (validação na borda MQTT)."""
from datetime import UTC, datetime

import pytest
from pydantic import ValidationError

from runs.schemas import EventoPayload, MazeCell, Pose, TelemetriaPayload, WallFlags


# ─── WallFlags ───────────────────────────────────────────────────────────────


def test_wall_flags_all_default_false():
    flags = WallFlags()
    assert flags.n is False
    assert flags.s is False
    assert flags.e is False
    assert flags.w is False


def test_wall_flags_set_all_true():
    flags = WallFlags(n=True, s=True, e=True, w=True)
    assert flags.model_dump() == {"n": True, "s": True, "e": True, "w": True}


# ─── MazeCell ────────────────────────────────────────────────────────────────


def test_maze_cell_valid():
    cell = MazeCell(x=3, y=4, walls=WallFlags(n=True))
    assert cell.x == 3
    assert cell.y == 4
    assert cell.walls.n is True


def test_maze_cell_invalid_missing_walls():
    with pytest.raises(ValidationError):
        MazeCell(x=0, y=0)


# ─── Pose ─────────────────────────────────────────────────────────────────────


def test_pose_valid_headings():
    for heading in ("N", "S", "E", "W"):
        pose = Pose(x=0, y=0, heading=heading)
        assert pose.heading == heading


def test_pose_invalid_heading():
    with pytest.raises(ValidationError):
        Pose(x=0, y=0, heading="X")


# ─── TelemetriaPayload ───────────────────────────────────────────────────────


def _valid_telemetria(**overrides) -> dict:
    base = {
        "ts": datetime.now(UTC).isoformat(),
        "run_id": "abc-123",
        "pose": {"x": 1, "y": 2, "heading": "N"},
        "maze_delta": [{"x": 1, "y": 2, "walls": {"n": True, "s": False, "e": False, "w": True}}],
        "speed": 0.3,
        "battery": 90.0,
        "voltage": 7.4,
    }
    base.update(overrides)
    return base


def test_telemetria_payload_valid():
    payload = TelemetriaPayload.model_validate(_valid_telemetria())
    assert payload.run_id == "abc-123"
    assert payload.pose.x == 1
    assert payload.pose.heading == "N"
    assert len(payload.maze_delta) == 1
    assert payload.speed == 0.3


def test_telemetria_payload_optional_fields_none():
    data = _valid_telemetria()
    del data["speed"]
    del data["battery"]
    del data["voltage"]
    payload = TelemetriaPayload.model_validate(data)
    assert payload.speed is None
    assert payload.battery is None
    assert payload.voltage is None


def test_telemetria_payload_empty_maze_delta():
    data = _valid_telemetria(maze_delta=[])
    payload = TelemetriaPayload.model_validate(data)
    assert payload.maze_delta == []


def test_telemetria_payload_invalid_heading():
    data = _valid_telemetria()
    data["pose"]["heading"] = "Z"
    with pytest.raises(ValidationError):
        TelemetriaPayload.model_validate(data)


def test_telemetria_payload_invalid_missing_run_id():
    data = _valid_telemetria()
    del data["run_id"]
    with pytest.raises(ValidationError):
        TelemetriaPayload.model_validate(data)


def test_telemetria_payload_invalid_missing_ts():
    data = _valid_telemetria()
    del data["ts"]
    with pytest.raises(ValidationError):
        TelemetriaPayload.model_validate(data)


# ─── EventoPayload ───────────────────────────────────────────────────────────


def _valid_evento(**overrides) -> dict:
    base = {
        "ts": datetime.now(UTC).isoformat(),
        "run_id": "run-999",
        "type": "inicio",
        "detail": "",
    }
    base.update(overrides)
    return base


def test_evento_payload_inicio_valid():
    payload = EventoPayload.model_validate(_valid_evento(type="inicio"))
    assert payload.type == "inicio"


def test_evento_payload_desafio_cumprido_valid():
    payload = EventoPayload.model_validate(_valid_evento(type="desafio_cumprido"))
    assert payload.type == "desafio_cumprido"


def test_evento_payload_colisao_valid():
    payload = EventoPayload.model_validate(_valid_evento(type="colisao"))
    assert payload.type == "colisao"


def test_evento_payload_reparo_valid():
    payload = EventoPayload.model_validate(_valid_evento(type="reparo"))
    assert payload.type == "reparo"


def test_evento_payload_invalid_type():
    with pytest.raises(ValidationError):
        EventoPayload.model_validate(_valid_evento(type="inexistente"))


def test_evento_payload_detail_defaults_to_empty_string():
    data = _valid_evento()
    del data["detail"]
    payload = EventoPayload.model_validate(data)
    assert payload.detail == ""

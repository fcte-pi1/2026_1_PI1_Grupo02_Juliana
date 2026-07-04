"""Tests para os serviços stateless de runs: maze.py, snapshot.py e funções
puras de simulator.py (sem MQTT).
"""
from unittest.mock import MagicMock, patch

import pytest

from runs.schemas import MazeCell, WallFlags
from runs.services.maze import apply_delta, cell_key
from runs.services.simulator import _boustrophedon, _direction, _heading, _walls_for
from runs.services.snapshot import build_snapshot
from runs.tests.factories import TentativaFactory


# ─── maze.py ─────────────────────────────────────────────────────────────────


def test_cell_key_formats_correctly():
    assert cell_key(2, 3) == "2,3"
    assert cell_key(0, 0) == "0,0"
    assert cell_key(15, 15) == "15,15"


def test_apply_delta_adds_new_cell():
    maze = {}
    cells = [MazeCell(x=1, y=2, walls=WallFlags(n=True, s=False, e=True, w=False))]
    result = apply_delta(maze, cells)
    assert result == {"1,2": {"n": True, "s": False, "e": True, "w": False}}


def test_apply_delta_overwrites_existing_cell():
    maze = {"0,0": {"n": False, "s": False, "e": False, "w": False}}
    cells = [MazeCell(x=0, y=0, walls=WallFlags(n=True, s=True, e=True, w=True))]
    apply_delta(maze, cells)
    assert maze["0,0"] == {"n": True, "s": True, "e": True, "w": True}


def test_apply_delta_returns_same_dict_instance():
    maze = {}
    cells = [MazeCell(x=0, y=0, walls=WallFlags())]
    result = apply_delta(maze, cells)
    assert result is maze


def test_apply_delta_empty_delta_does_not_change_maze():
    maze = {"0,0": {"n": True, "s": False, "e": False, "w": True}}
    result = apply_delta(maze, [])
    assert result == {"0,0": {"n": True, "s": False, "e": False, "w": True}}


def test_apply_delta_adds_multiple_cells():
    maze = {}
    cells = [
        MazeCell(x=0, y=0, walls=WallFlags(n=True)),
        MazeCell(x=1, y=0, walls=WallFlags(s=True)),
        MazeCell(x=0, y=1, walls=WallFlags(e=True)),
    ]
    apply_delta(maze, cells)
    assert len(maze) == 3
    assert "0,0" in maze
    assert "1,0" in maze
    assert "0,1" in maze


# ─── snapshot.py ─────────────────────────────────────────────────────────────


@pytest.mark.django_db
def test_build_snapshot_returns_canonical_shape():
    tentativa = TentativaFactory(maze={"0,0": {"n": True, "s": False, "e": False, "w": True}})
    snapshot = build_snapshot(tentativa)
    assert snapshot["tentativa_id"] == str(tentativa.id)
    assert snapshot["dimensao"] == 16
    assert snapshot["status"] == tentativa.status
    assert snapshot["explored"] == 1
    assert snapshot["maze"] == {"0,0": {"n": True, "s": False, "e": False, "w": True}}


@pytest.mark.django_db
def test_build_snapshot_with_live_values():
    tentativa = TentativaFactory()
    snapshot = build_snapshot(tentativa, speed=0.5, battery=85.0, voltage=7.2, ts="ts-value")
    assert snapshot["speed"] == 0.5
    assert snapshot["battery"] == 85.0
    assert snapshot["voltage"] == 7.2
    assert snapshot["ts"] == "ts-value"


@pytest.mark.django_db
def test_build_snapshot_battery_falls_back_to_consumo_bateria():
    tentativa = TentativaFactory()
    tentativa.consumo_bateria = 75.0
    tentativa.save()
    snapshot = build_snapshot(tentativa)
    assert snapshot["battery"] == 75.0


@pytest.mark.django_db
def test_build_snapshot_empty_maze():
    tentativa = TentativaFactory()
    snapshot = build_snapshot(tentativa)
    assert snapshot["explored"] == 0
    assert snapshot["maze"] == {}


@pytest.mark.django_db
def test_build_snapshot_pose_none_when_empty():
    tentativa = TentativaFactory()
    snapshot = build_snapshot(tentativa)
    assert snapshot["pose"] is None


@pytest.mark.django_db
def test_build_snapshot_pose_populated():
    tentativa = TentativaFactory()
    tentativa.pose = {"x": 3, "y": 4, "heading": "E"}
    tentativa.save()
    snapshot = build_snapshot(tentativa)
    assert snapshot["pose"] == {"x": 3, "y": 4, "heading": "E"}


# ─── simulator.py — funções puras ────────────────────────────────────────────


def test_direction_north():
    assert _direction((0, 1), (0, 0)) == "n"


def test_direction_south():
    assert _direction((0, 0), (0, 1)) == "s"


def test_direction_east():
    assert _direction((0, 0), (1, 0)) == "e"


def test_direction_west():
    assert _direction((1, 0), (0, 0)) == "w"


def test_heading_mapping():
    assert _heading("n") == "N"
    assert _heading("s") == "S"
    assert _heading("e") == "E"
    assert _heading("w") == "W"


def test_boustrophedon_small_grid_contains_origin():
    path = _boustrophedon(4, 100)
    assert path[0] == (0, 0)


def test_boustrophedon_truncates_at_max_steps():
    path = _boustrophedon(4, 3)
    assert len(path) <= 3


def test_boustrophedon_stops_at_center():
    # 4x4 grid: centre = (2,2). Path must end there or earlier.
    path = _boustrophedon(4, 100)
    assert (2, 2) in path
    assert path[-1] == (2, 2)


def test_boustrophedon_even_rows_go_left_to_right():
    path = _boustrophedon(4, 100)
    row0 = [(x, y) for x, y in path if y == 0]
    xs = [x for x, _ in row0]
    assert xs == sorted(xs)


def test_boustrophedon_odd_rows_go_right_to_left():
    path = _boustrophedon(4, 100)
    # Row 1 should be reversed
    row1 = [(x, y) for x, y in path if y == 1]
    if len(row1) > 1:
        xs = [x for x, _ in row1]
        assert xs == sorted(xs, reverse=True)


def test_walls_for_no_neighbors_all_walls_closed():
    walls = _walls_for((0, 0), None, None)
    assert all(walls[d] for d in ("n", "s", "e", "w"))


def test_walls_for_open_to_next_east():
    walls = _walls_for((0, 0), None, (1, 0))
    assert walls["e"] is False
    assert walls["n"] is True
    assert walls["s"] is True
    assert walls["w"] is True


def test_walls_for_open_to_prev_and_next():
    walls = _walls_for((1, 0), (0, 0), (2, 0))
    assert walls["e"] is False  # open to next (east)
    assert walls["w"] is False  # open to prev (west)
    assert walls["n"] is True
    assert walls["s"] is True


# ─── simulator.py — run_simulation (mock MQTT) ───────────────────────────────


@pytest.mark.django_db
def test_run_simulation_completes_and_returns_true(monkeypatch):
    from runs.services.simulator import run_simulation

    mock_client = MagicMock()
    monkeypatch.setattr("runs.services.simulator.build_client", lambda **kw: mock_client)
    monkeypatch.setattr("runs.services.simulator.time.sleep", lambda _: None)

    tentativa = TentativaFactory()
    result = run_simulation(tentativa=tentativa, hz=100.0, steps=3)

    assert result is True
    mock_client.loop_start.assert_called_once()
    mock_client.loop_stop.assert_called_once()
    mock_client.disconnect.assert_called_once()


@pytest.mark.django_db
def test_run_simulation_stops_early_when_should_stop(monkeypatch):
    from runs.services.simulator import run_simulation

    mock_client = MagicMock()
    monkeypatch.setattr("runs.services.simulator.build_client", lambda **kw: mock_client)
    monkeypatch.setattr("runs.services.simulator.time.sleep", lambda _: None)

    tentativa = TentativaFactory()
    result = run_simulation(tentativa=tentativa, hz=100.0, steps=10, should_stop=lambda: True)

    assert result is False


@pytest.mark.django_db
def test_run_simulation_calls_log(monkeypatch):
    from runs.services.simulator import run_simulation

    mock_client = MagicMock()
    monkeypatch.setattr("runs.services.simulator.build_client", lambda **kw: mock_client)
    monkeypatch.setattr("runs.services.simulator.time.sleep", lambda _: None)

    log_messages = []
    tentativa = TentativaFactory()
    run_simulation(tentativa=tentativa, hz=100.0, steps=2, log=log_messages.append)

    assert len(log_messages) > 0

"""Testes dos use cases de telemetria."""
from datetime import UTC, datetime

import pytest

from runs.models import Posicao, Tentativa
from runs.tests.factories import TentativaFactory
from runs.use_cases.persistir_telemetria import PersistirTelemetria
from runs.use_cases.registrar_evento import RegistrarEvento


def _telemetria(run_id: str, x: int, y: int, walls: dict) -> dict:
    return {
        "ts": datetime.now(UTC).isoformat(),
        "run_id": run_id,
        "pose": {"x": x, "y": y, "heading": "N"},
        "maze_delta": [{"x": x, "y": y, "walls": walls}],
        "speed": 0.3,
        "battery": 90.0,
        "voltage": 7.3,
    }


@pytest.mark.django_db
def test_persistir_telemetria_aplica_delta_e_cria_posicao():
    tentativa = TentativaFactory()
    payload = _telemetria(str(tentativa.id), 2, 3, {"n": True, "s": False, "e": True, "w": False})

    snapshot = PersistirTelemetria().execute(payload=payload)

    tentativa.refresh_from_db()
    assert tentativa.maze == {"2,3": {"n": True, "s": False, "e": True, "w": False}}
    assert tentativa.pose == {"x": 2, "y": 3, "heading": "N"}
    assert Posicao.objects.filter(tentativa=tentativa).count() == 1
    assert snapshot["explored"] == 1
    assert snapshot["dimensao"] == 16
    assert snapshot["speed"] == 0.3


@pytest.mark.django_db
def test_persistir_telemetria_acumula_celulas():
    tentativa = TentativaFactory()
    rid = str(tentativa.id)
    walls = {"n": True, "s": False, "e": False, "w": True}
    PersistirTelemetria().execute(payload=_telemetria(rid, 0, 0, walls))
    PersistirTelemetria().execute(payload=_telemetria(rid, 1, 0, walls))

    tentativa.refresh_from_db()
    assert set(tentativa.maze.keys()) == {"0,0", "1,0"}
    assert Posicao.objects.filter(tentativa=tentativa).count() == 2


@pytest.mark.django_db
def test_registrar_evento_desafio_cumprido_fecha_tentativa():
    tentativa = TentativaFactory()
    payload = {
        "ts": datetime.now(UTC).isoformat(),
        "run_id": str(tentativa.id),
        "type": "desafio_cumprido",
        "detail": "",
    }

    RegistrarEvento().execute(payload=payload)

    tentativa.refresh_from_db()
    assert tentativa.status == Tentativa.Status.FINALIZADA
    assert tentativa.sucesso is True
    assert tentativa.tempo_fim is not None

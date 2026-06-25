"""Testes dos use cases de telemetria e movimentação."""
from datetime import UTC, datetime

import pytest

from runs.models import Posicao, Tentativa
from runs.tests.factories import TentativaFactory
from runs.use_cases.girar import Girar
from runs.use_cases.mover_frente import MoverFrente
from runs.use_cases.persistir_telemetria import PersistirTelemetria
from runs.use_cases.registrar_evento import RegistrarEvento


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _fake_mqtt_client(chamadas: dict):
    class _Client:
        def publish(self, topic, payload, qos=0):
            chamadas["topic"] = topic
            chamadas["payload"] = payload

        def disconnect(self):
            pass

    return _Client()


def _patch_mqtt(monkeypatch, chamadas: dict):
    monkeypatch.setattr(
        "runs.use_cases.mover_frente.build_client",
        lambda **kw: _fake_mqtt_client(chamadas),
    )


# ---------------------------------------------------------------------------
# MoverFrente (RF01)
# ---------------------------------------------------------------------------


@pytest.mark.django_db
def test_mover_frente_publica_comando_correto(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt(monkeypatch, chamadas)

    result = MoverFrente().execute(tentativa_id=str(tentativa.id), velocidade=180)

    assert result["velocidade"] == 180
    assert chamadas["payload"] == "FRENTE 180"
    assert str(tentativa.id) in chamadas["topic"]


@pytest.mark.django_db
def test_mover_frente_usa_velocidade_padrao(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt(monkeypatch, chamadas)

    MoverFrente().execute(tentativa_id=str(tentativa.id))

    assert chamadas["payload"] == f"FRENTE {MoverFrente.VELOCIDADE_PADRAO}"


@pytest.mark.django_db
def test_mover_frente_limita_velocidade_maxima(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt(monkeypatch, chamadas)

    result = MoverFrente().execute(tentativa_id=str(tentativa.id), velocidade=999)

    assert result["velocidade"] == MoverFrente.VELOCIDADE_MAX
    assert chamadas["payload"] == f"FRENTE {MoverFrente.VELOCIDADE_MAX}"


@pytest.mark.django_db
def test_mover_frente_limita_velocidade_minima(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt(monkeypatch, chamadas)

    result = MoverFrente().execute(tentativa_id=str(tentativa.id), velocidade=0)

    assert result["velocidade"] == 1
    assert chamadas["payload"] == "FRENTE 1"


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


# ---------------------------------------------------------------------------
# Girar (RF02)
# ---------------------------------------------------------------------------


@pytest.mark.django_db
def test_girar_publica_comando_correto_90_graus(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt_girar(monkeypatch, chamadas)

    result = Girar().execute(tentativa_id=str(tentativa.id), angulo=90)

    assert result["angulo"] == 90
    assert chamadas["payload"] == "GIRAR 90"
    assert str(tentativa.id) in chamadas["topic"]


@pytest.mark.django_db
def test_girar_publica_comando_correto_menos_90(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt_girar(monkeypatch, chamadas)

    result = Girar().execute(tentativa_id=str(tentativa.id), angulo=-90)

    assert result["angulo"] == -90
    assert chamadas["payload"] == "GIRAR -90"


@pytest.mark.django_db
def test_girar_publica_comando_correto_180(monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    _patch_mqtt_girar(monkeypatch, chamadas)

    result = Girar().execute(tentativa_id=str(tentativa.id), angulo=180)

    assert result["angulo"] == 180
    assert chamadas["payload"] == "GIRAR 180"


@pytest.mark.django_db
def test_girar_angulo_invalido_levanta_excecao(monkeypatch):
    tentativa = TentativaFactory()
    _patch_mqtt_girar(monkeypatch, {})

    with pytest.raises(ValueError, match="45"):
        Girar().execute(tentativa_id=str(tentativa.id), angulo=45)


def _patch_mqtt_girar(monkeypatch, chamadas: dict):
    class _FakeClient:
        def publish(self, topic, payload, qos=0):
            chamadas["topic"] = topic
            chamadas["payload"] = payload

        def disconnect(self):
            pass

    monkeypatch.setattr(
        "runs.use_cases.girar.build_client", lambda **kw: _FakeClient()
    )


# ---------------------------------------------------------------------------
# Telemetria
# ---------------------------------------------------------------------------


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

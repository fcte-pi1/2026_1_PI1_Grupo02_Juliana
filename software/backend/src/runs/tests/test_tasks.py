"""Tests para runs.tasks — parar_corrida e processar_telemetria.

Redis e MQTT são mockados para não exigir infra externa.
CELERY_TASK_ALWAYS_EAGER=True (settings de teste) faz tasks rodarem síncronas.
"""
from datetime import UTC, datetime
from unittest.mock import MagicMock, patch

import pytest

from runs.tasks import _stop_key, parar_corrida, processar_telemetria
from runs.tests.factories import TentativaFactory


# ─── _stop_key ───────────────────────────────────────────────────────────────


def test_stop_key_format():
    assert _stop_key("abc-123") == "sim:stop:abc-123"


# ─── parar_corrida ───────────────────────────────────────────────────────────


def test_parar_corrida_sets_redis_flag():
    mock_redis = MagicMock()
    with patch("runs.tasks._redis", return_value=mock_redis):
        parar_corrida("run-xyz")

    mock_redis.set.assert_called_once_with("sim:stop:run-xyz", "1", ex=600)


# ─── processar_telemetria ────────────────────────────────────────────────────


def _telemetria_payload(run_id: str) -> dict:
    return {
        "ts": datetime.now(UTC).isoformat(),
        "run_id": run_id,
        "pose": {"x": 1, "y": 1, "heading": "N"},
        "maze_delta": [{"x": 1, "y": 1, "walls": {"n": True, "s": False, "e": False, "w": True}}],
        "speed": 0.3,
        "battery": 90.0,
        "voltage": 7.4,
    }


@pytest.mark.django_db
def test_processar_telemetria_returns_success(monkeypatch):
    tentativa = TentativaFactory()
    payload = _telemetria_payload(str(tentativa.id))

    monkeypatch.setattr("runs.tasks.realtime.publish_snapshot", lambda tid, snap: None)
    monkeypatch.setattr("runs.tasks.get_channel_layer", lambda: None)

    result = processar_telemetria(payload)
    assert result["success"] is True
    assert "snapshot" in result


@pytest.mark.django_db
def test_processar_telemetria_persists_posicao(monkeypatch):
    from runs.models import Posicao

    tentativa = TentativaFactory()
    payload = _telemetria_payload(str(tentativa.id))

    monkeypatch.setattr("runs.tasks.realtime.publish_snapshot", lambda tid, snap: None)
    monkeypatch.setattr("runs.tasks.get_channel_layer", lambda: None)

    processar_telemetria(payload)

    assert Posicao.objects.filter(tentativa=tentativa).count() == 1


@pytest.mark.django_db
def test_processar_telemetria_calls_publish_snapshot(monkeypatch):
    tentativa = TentativaFactory()
    payload = _telemetria_payload(str(tentativa.id))

    published = {}
    monkeypatch.setattr(
        "runs.tasks.realtime.publish_snapshot",
        lambda tid, snap: published.update({"tid": tid, "snap": snap}),
    )
    monkeypatch.setattr("runs.tasks.get_channel_layer", lambda: None)

    processar_telemetria(payload)

    assert published["tid"] == str(tentativa.id)
    assert isinstance(published["snap"], dict)


@pytest.mark.django_db
def test_processar_telemetria_sends_to_channel_layer(monkeypatch):
    tentativa = TentativaFactory()
    payload = _telemetria_payload(str(tentativa.id))

    monkeypatch.setattr("runs.tasks.realtime.publish_snapshot", lambda tid, snap: None)

    mock_layer = MagicMock()
    sent_events = []

    def fake_group_send(group, event):
        sent_events.append((group, event))

    mock_layer.group_send = fake_group_send

    with patch("runs.tasks.async_to_sync", return_value=fake_group_send):
        monkeypatch.setattr("runs.tasks.get_channel_layer", lambda: mock_layer)
        processar_telemetria(payload)

    assert len(sent_events) == 1
    assert sent_events[0][0] == "telemetry"


# ─── simular_corrida ─────────────────────────────────────────────────────────


@pytest.mark.django_db
def test_simular_corrida_returns_true_when_completed(monkeypatch):
    from runs.tasks import simular_corrida

    tentativa = TentativaFactory()

    mock_redis = MagicMock()
    mock_redis.exists.return_value = False

    monkeypatch.setattr("runs.tasks._redis", lambda: mock_redis)
    monkeypatch.setattr(
        "runs.tasks.run_simulation",
        lambda **kw: True,
    )

    result = simular_corrida(str(tentativa.id))
    assert result is True


@pytest.mark.django_db
def test_simular_corrida_sets_abortada_when_stopped(monkeypatch):
    from runs.models import Tentativa
    from runs.tasks import simular_corrida

    tentativa = TentativaFactory()

    mock_redis = MagicMock()
    mock_redis.exists.return_value = False

    monkeypatch.setattr("runs.tasks._redis", lambda: mock_redis)
    monkeypatch.setattr(
        "runs.tasks.run_simulation",
        lambda **kw: False,
    )

    result = simular_corrida(str(tentativa.id))
    assert result is False
    tentativa.refresh_from_db()
    assert tentativa.status == Tentativa.Status.ABORTADA

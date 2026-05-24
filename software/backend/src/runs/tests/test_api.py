"""Smoke tests da API de runs — envelope, snapshot e comando."""
import pytest

from runs.tests.factories import TentativaFactory


@pytest.mark.django_db
def test_list_tentativas_unauthenticated_returns_401(api_client):
    response = api_client.get("/api/v1/runs/tentativas/")
    assert response.status_code == 401


@pytest.mark.django_db
def test_list_tentativas_returns_envelope(auth_client):
    TentativaFactory.create_batch(2)
    response = auth_client.get("/api/v1/runs/tentativas/")
    assert response.status_code == 200
    body = response.json()
    assert body["success"] is True
    assert len(body["data"]) == 2


@pytest.mark.django_db
def test_snapshot_action_returns_maze_and_dimensao(auth_client):
    tentativa = TentativaFactory(maze={"0,0": {"n": True, "s": False, "e": False, "w": True}})
    response = auth_client.get(f"/api/v1/runs/tentativas/{tentativa.id}/snapshot/")
    assert response.status_code == 200
    data = response.json()["data"]
    assert data["dimensao"] == 16
    assert data["explored"] == 1
    assert data["maze"] == {"0,0": {"n": True, "s": False, "e": False, "w": True}}


@pytest.mark.django_db
def test_comando_start_dispara_simulacao(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    called = {}
    monkeypatch.setattr(
        "runs.api.views.simular_corrida.delay", lambda run_id: called.setdefault("start", run_id)
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/", {"acao": "start"}, format="json"
    )
    assert response.status_code == 202
    assert called["start"] == str(tentativa.id)


@pytest.mark.django_db
def test_comando_stop_sinaliza_parada(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    called = {}
    monkeypatch.setattr(
        "runs.api.views.parar_corrida", lambda run_id: called.setdefault("stop", run_id)
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/", {"acao": "stop"}, format="json"
    )
    assert response.status_code == 202
    assert called["stop"] == str(tentativa.id)

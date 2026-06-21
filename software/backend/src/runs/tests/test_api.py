"""Smoke tests da API de runs — envelope, snapshot e comando."""
import pytest

from runs.tests.factories import TentativaFactory


def _fake_mqtt_client(chamadas: dict):
    class _Client:
        def publish(self, topic, payload, qos=0):
            chamadas["topic"] = topic
            chamadas["payload"] = payload

        def disconnect(self):
            pass

    return _Client()


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


@pytest.mark.django_db
def test_comando_mover_frente_retorna_202_e_publica_mqtt(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    monkeypatch.setattr(
        "runs.use_cases.mover_frente.build_client",
        lambda **kw: _fake_mqtt_client(chamadas),
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "mover_frente", "velocidade": 180},
        format="json",
    )

    assert response.status_code == 202
    assert chamadas["payload"] == "FRENTE 180"
    assert str(tentativa.id) in chamadas["topic"]


@pytest.mark.django_db
def test_comando_mover_frente_usa_velocidade_padrao(auth_client, monkeypatch):
    from runs.use_cases.mover_frente import MoverFrente

    tentativa = TentativaFactory()
    chamadas: dict = {}
    monkeypatch.setattr(
        "runs.use_cases.mover_frente.build_client",
        lambda **kw: _fake_mqtt_client(chamadas),
    )

    auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "mover_frente"},
        format="json",
    )

    assert chamadas["payload"] == f"FRENTE {MoverFrente.VELOCIDADE_PADRAO}"


# ---------------------------------------------------------------------------
# RF02 — girar
# ---------------------------------------------------------------------------


@pytest.mark.django_db
def test_comando_girar_90_retorna_202_e_publica_mqtt(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    monkeypatch.setattr(
        "runs.use_cases.girar.build_client",
        lambda **kw: _fake_mqtt_client(chamadas),
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "girar", "angulo": 90},
        format="json",
    )

    assert response.status_code == 202
    assert chamadas["payload"] == "GIRAR 90"
    assert str(tentativa.id) in chamadas["topic"]


@pytest.mark.django_db
def test_comando_girar_menos_90_publica_mqtt(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    monkeypatch.setattr(
        "runs.use_cases.girar.build_client",
        lambda **kw: _fake_mqtt_client(chamadas),
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "girar", "angulo": -90},
        format="json",
    )

    assert response.status_code == 202
    assert chamadas["payload"] == "GIRAR -90"


@pytest.mark.django_db
def test_comando_girar_180_publica_mqtt(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    chamadas: dict = {}
    monkeypatch.setattr(
        "runs.use_cases.girar.build_client",
        lambda **kw: _fake_mqtt_client(chamadas),
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "girar", "angulo": 180},
        format="json",
    )

    assert response.status_code == 202
    assert chamadas["payload"] == "GIRAR 180"


@pytest.mark.django_db
def test_comando_girar_angulo_invalido_retorna_400(auth_client, monkeypatch):
    tentativa = TentativaFactory()
    monkeypatch.setattr(
        "runs.use_cases.girar.build_client",
        lambda **kw: _fake_mqtt_client({}),
    )

    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "girar", "angulo": 45},
        format="json",
    )

    assert response.status_code == 400
    assert "45" in response.text  # mensagem de erro contém o ângulo inválido

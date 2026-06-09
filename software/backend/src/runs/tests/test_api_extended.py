"""Testes adicionais da API de runs — cobrindo endpoints não testados em test_api.py."""
from datetime import UTC, datetime

import pytest

from runs.models import Posicao, Tentativa
from runs.tests.factories import LabirintoFactory, MicromouseFactory, TentativaFactory


# ─── Micromouse ───────────────────────────────────────────────────────────────


@pytest.mark.django_db
def test_list_micromouses_unauthenticated_returns_401(api_client):
    response = api_client.get("/api/v1/runs/micromouses/")
    assert response.status_code == 401


@pytest.mark.django_db
def test_list_micromouses_returns_envelope(auth_client):
    MicromouseFactory.create_batch(2)
    response = auth_client.get("/api/v1/runs/micromouses/")
    assert response.status_code == 200
    body = response.json()
    assert body["success"] is True
    assert len(body["data"]) == 2


@pytest.mark.django_db
def test_retrieve_micromouse_returns_detail(auth_client):
    mm = MicromouseFactory(nome="TestMouse", algoritmo="FloodFill")
    response = auth_client.get(f"/api/v1/runs/micromouses/{mm.id}/")
    assert response.status_code == 200
    data = response.json()["data"]
    assert data["nome"] == "TestMouse"
    assert data["algoritmo"] == "FloodFill"


# ─── Labirinto ────────────────────────────────────────────────────────────────


@pytest.mark.django_db
def test_list_labirintos_unauthenticated_returns_401(api_client):
    response = api_client.get("/api/v1/runs/labirintos/")
    assert response.status_code == 401


@pytest.mark.django_db
def test_list_labirintos_returns_envelope(auth_client):
    LabirintoFactory.create_batch(3)
    response = auth_client.get("/api/v1/runs/labirintos/")
    assert response.status_code == 200
    body = response.json()
    assert body["success"] is True
    assert len(body["data"]) == 3


@pytest.mark.django_db
def test_retrieve_labirinto_returns_detail(auth_client):
    lab = LabirintoFactory(nome="Maze16", dimensao=16)
    response = auth_client.get(f"/api/v1/runs/labirintos/{lab.id}/")
    assert response.status_code == 200
    data = response.json()["data"]
    assert data["nome"] == "Maze16"
    assert data["dimensao"] == 16


# ─── Tentativa — posicoes e trajetoria ───────────────────────────────────────


@pytest.mark.django_db
def test_posicoes_returns_empty_list_when_no_positions(auth_client):
    tentativa = TentativaFactory()
    response = auth_client.get(f"/api/v1/runs/tentativas/{tentativa.id}/posicoes/")
    assert response.status_code == 200
    body = response.json()
    assert body["success"] is True
    assert body["data"] == []


@pytest.mark.django_db
def test_posicoes_returns_ordered_positions(auth_client):
    tentativa = TentativaFactory()
    now = datetime.now(UTC)
    Posicao.objects.create(tentativa=tentativa, coordenada_x=1, coordenada_y=1, timestamp=now, passo=2)
    Posicao.objects.create(tentativa=tentativa, coordenada_x=0, coordenada_y=0, timestamp=now, passo=1)

    response = auth_client.get(f"/api/v1/runs/tentativas/{tentativa.id}/posicoes/")
    assert response.status_code == 200
    data = response.json()["data"]
    assert len(data) == 2
    assert data[0]["passo"] == 1
    assert data[1]["passo"] == 2


@pytest.mark.django_db
def test_trajetoria_returns_empty_list_when_no_positions(auth_client):
    tentativa = TentativaFactory()
    response = auth_client.get(f"/api/v1/runs/tentativas/{tentativa.id}/trajetoria/")
    assert response.status_code == 200
    body = response.json()
    assert body["success"] is True
    assert body["data"] == []


@pytest.mark.django_db
def test_trajetoria_returns_correct_shape(auth_client):
    tentativa = TentativaFactory()
    now = datetime.now(UTC)
    Posicao.objects.create(
        tentativa=tentativa,
        coordenada_x=3, coordenada_y=4,
        timestamp=now, passo=1,
        orientacao="E", velocidade=0.4, bateria=88.0,
    )

    response = auth_client.get(f"/api/v1/runs/tentativas/{tentativa.id}/trajetoria/")
    assert response.status_code == 200
    data = response.json()["data"]
    assert len(data) == 1
    item = data[0]
    assert item["x"] == 3
    assert item["y"] == 4
    assert item["step"] == 1
    assert item["orientation"] == "E"
    assert item["velocity"] == 0.4
    assert item["battery"] == 88.0


# ─── Tentativa — filtros ─────────────────────────────────────────────────────


@pytest.mark.django_db
def test_filter_tentativas_by_status(auth_client):
    TentativaFactory(status=Tentativa.Status.EM_CURSO)
    TentativaFactory(status=Tentativa.Status.FINALIZADA)
    TentativaFactory(status=Tentativa.Status.ABORTADA)

    response = auth_client.get("/api/v1/runs/tentativas/?status=em_curso")
    assert response.status_code == 200
    body = response.json()
    assert all(item["status"] == "em_curso" for item in body["data"])
    assert len(body["data"]) == 1


@pytest.mark.django_db
def test_filter_tentativas_by_sucesso(auth_client):
    TentativaFactory(sucesso=True)
    TentativaFactory(sucesso=False)
    TentativaFactory(sucesso=None)

    response = auth_client.get("/api/v1/runs/tentativas/?sucesso=true")
    assert response.status_code == 200
    body = response.json()
    assert all(item["sucesso"] is True for item in body["data"])


@pytest.mark.django_db
def test_retrieve_tentativa_returns_serializer_fields(auth_client):
    tentativa = TentativaFactory(
        maze={"0,0": {"n": True, "s": False, "e": False, "w": True}},
        pose={"x": 0, "y": 0, "heading": "N"},
    )
    response = auth_client.get(f"/api/v1/runs/tentativas/{tentativa.id}/")
    assert response.status_code == 200
    data = response.json()["data"]
    assert "dimensao" in data
    assert "explored" in data
    assert data["explored"] == 1


# ─── Tentativa — comando ação inválida ───────────────────────────────────────


@pytest.mark.django_db
def test_comando_acao_invalida_retorna_202_com_acao_none(auth_client):
    tentativa = TentativaFactory()
    response = auth_client.post(
        f"/api/v1/runs/tentativas/{tentativa.id}/comando/",
        {"acao": "reset"},
        format="json",
    )
    assert response.status_code == 202
    body = response.json()
    assert body["data"]["acao"] == "reset"

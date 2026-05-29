"""Smoke tests da API do `ServiceCheck` — envelope, paginação, filtros, action `run`."""
import pytest

from healthcheck.tests.factories import ServiceCheckFactory
from integrations.fetcher.base import FetchResult


class FakeFetcher:
    def __init__(self, status_code: int = 200):
        self.status_code = status_code

    def get(self, url: str) -> FetchResult:
        return FetchResult(status_code=self.status_code, body="", headers={})


@pytest.mark.django_db
def test_list_checks_unauthenticated_returns_401(api_client):
    response = api_client.get("/api/v1/healthcheck/checks/")
    assert response.status_code == 401


@pytest.mark.django_db
def test_list_checks_returns_envelope_with_pagination(auth_client):
    ServiceCheckFactory.create_batch(3)
    response = auth_client.get("/api/v1/healthcheck/checks/")
    assert response.status_code == 200

    body = response.json()
    assert body["success"] is True
    assert isinstance(body["data"], list)
    assert len(body["data"]) == 3
    assert body["pagination"]["count"] == 3
    assert body["pagination"]["page_size"] == 20
    assert body["pagination"]["page"] == 1
    assert body["pagination"]["total_pages"] == 1


@pytest.mark.django_db
def test_pagination_page_size_query_param_respected(auth_client):
    """`?page_size=N` deve refletir no envelope (até o cap `max_page_size`)."""
    ServiceCheckFactory.create_batch(10)

    response = auth_client.get("/api/v1/healthcheck/checks/?page_size=5")
    assert response.status_code == 200
    body = response.json()
    assert body["pagination"]["page_size"] == 5
    assert len(body["data"]) == 5

    # page_size acima de max_page_size (200) é capado.
    response = auth_client.get("/api/v1/healthcheck/checks/?page_size=999")
    assert response.status_code == 200
    body = response.json()
    assert body["pagination"]["page_size"] == 200


@pytest.mark.django_db
def test_create_check_returns_201_envelope(auth_client):
    payload = {
        "name": "micromouse-api",
        "url": "https://api.micromouse.dev/health",
        "expected_status": 200,
        "interval_seconds": 60,
        "is_active": True,
    }
    response = auth_client.post("/api/v1/healthcheck/checks/", payload, format="json")
    assert response.status_code == 201

    body = response.json()
    assert body["success"] is True
    assert body["data"]["name"] == "micromouse-api"
    assert body["data"]["url"] == "https://api.micromouse.dev/health"


@pytest.mark.django_db
def test_run_action_executes_check(auth_client, monkeypatch):
    check = ServiceCheckFactory(expected_status=200)

    # A view instancia `RunCheck()` sem fetcher, o que cairia no HttpxFetcher real.
    # Mockamos `get_fetcher` no módulo do use case pra evitar rede.
    monkeypatch.setattr(
        "healthcheck.use_cases.run_check.get_fetcher",
        lambda **kw: FakeFetcher(200),
    )

    response = auth_client.post(f"/api/v1/healthcheck/checks/{check.id}/run/")
    assert response.status_code == 200

    body = response.json()
    assert body["success"] is True
    assert body["data"]["last_status"] == "ok"
    assert body["data"]["last_status_code"] == 200


@pytest.mark.django_db
def test_filterset_is_active(auth_client):
    ServiceCheckFactory.create_batch(2, is_active=True)
    ServiceCheckFactory(is_active=False)

    response = auth_client.get("/api/v1/healthcheck/checks/?is_active=true")
    assert response.status_code == 200

    body = response.json()
    assert body["success"] is True
    assert all(item["is_active"] is True for item in body["data"])
    assert len(body["data"]) == 2

"""Smoke tests do JWT — obtain + refresh, lendo via envelope."""
import pytest

from core.tests.factories import UserFactory


@pytest.mark.django_db
def test_token_obtain_returns_access_and_refresh(api_client):
    user = UserFactory()
    response = api_client.post(
        "/api/v1/token/",
        {"username": user.username, "password": "x"},
        format="json",
    )
    assert response.status_code == 200

    body = response.json()
    assert body["success"] is True
    assert "access" in body["data"]
    assert "refresh" in body["data"]
    assert body["data"]["access"]
    assert body["data"]["refresh"]


@pytest.mark.django_db
def test_token_refresh_returns_new_access(api_client):
    user = UserFactory()
    obtain = api_client.post(
        "/api/v1/token/",
        {"username": user.username, "password": "x"},
        format="json",
    )
    assert obtain.status_code == 200
    refresh_token = obtain.json()["data"]["refresh"]

    response = api_client.post(
        "/api/v1/token/refresh/",
        {"refresh": refresh_token},
        format="json",
    )
    assert response.status_code == 200
    body = response.json()
    assert body["success"] is True
    assert body["data"]["access"]

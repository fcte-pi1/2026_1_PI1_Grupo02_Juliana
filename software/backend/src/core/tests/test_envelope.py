"""Smoke tests — health endpoint + RequestID middleware.

Healthcheck retorna JSON plano (não envelope) — shape canônico esperado por
probes externos (Uptime Kuma, K8s liveness). Cobertura completa do endpoint
está em ``test_health_endpoint.py``; aqui só validamos:

* status 200 + payload básico no happy path,
* ``X-Request-ID`` header sai mesmo em response que pula EnvelopeRenderer.

Cobertura do envelope canônico vive nos testes dos apps de domínio
(``healthcheck/tests/test_*.py`` etc), que usam endpoints DRF reais.
"""
from unittest.mock import MagicMock, patch

import pytest

from core.observability import health as health_module


def _all_ok():
    return patch.multiple(
        health_module,
        _check_db=MagicMock(return_value={"status": "ok"}),
        _check_redis=MagicMock(return_value={"status": "ok"}),
        _check_minio=MagicMock(return_value={"status": "ok"}),
    )


def test_health_endpoint_returns_plain_json_ok(api_client):
    """Health não usa envelope DRF — JSON plano pra probes externos."""
    with _all_ok():
        response = api_client.get("/api/v1/health/")
    assert response.status_code == 200
    body = response.json()
    assert body["status"] == "ok"
    assert "checks" in body


def test_request_id_header_present_on_health(api_client):
    """RequestIDMiddleware roda mesmo em endpoints fora do DRF."""
    with _all_ok():
        response = api_client.get("/api/v1/health/")
    assert response.status_code == 200
    assert response["X-Request-ID"]


def test_envelope_204_returns_empty_body():
    pytest.skip("nenhum endpoint 204 disponível no template ainda")

"""Testes do endpoint ``GET /api/v1/health/``.

Cobre os 3 checks (db/redis/minio), agregação de status, HTTP code,
ausência de auth e comportamento permissivo em ``skipped``.

Os checks tocam Postgres/Redis/MinIO — mockamos cada um pra que a suite
rode sem infra externa.
"""
from unittest.mock import MagicMock, patch

from django.urls import reverse

from core.observability import health as health_module


def _patch_all_healthy():
    """Context manager que força todos os checks retornando ``ok``."""
    return patch.multiple(
        health_module,
        _check_db=MagicMock(return_value={"status": health_module.STATUS_OK}),
        _check_redis=MagicMock(return_value={"status": health_module.STATUS_OK}),
        _check_minio=MagicMock(return_value={"status": health_module.STATUS_OK}),
    )


# ─── Routing / auth ──────────────────────────────────────────────────────────


class TestHealthRouting:
    def test_reverse_resolves(self):
        assert reverse("health") == "/api/v1/health/"

    def test_requires_no_auth(self, api_client):
        with _patch_all_healthy():
            response = api_client.get("/api/v1/health/")
        assert response.status_code == 200


# ─── Payload + agregação (via APIClient) ─────────────────────────────────────


class TestHealthPayload:
    """Healthcheck retorna JSON plano (sem envelope DRF) — shape canônico."""

    def test_health_endpoint_returns_ok_when_all_checks_pass(self, api_client):
        with _patch_all_healthy():
            response = api_client.get("/api/v1/health/")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "ok"
        assert data["checks"]["db"]["status"] == "ok"
        assert data["checks"]["redis"]["status"] == "ok"
        assert data["checks"]["minio"]["status"] == "ok"

    def test_health_endpoint_returns_503_when_db_fails(self, api_client):
        with patch.multiple(
            health_module,
            _check_db=MagicMock(
                return_value={"status": health_module.STATUS_FAIL, "error": "connection refused"}
            ),
            _check_redis=MagicMock(return_value={"status": health_module.STATUS_OK}),
            _check_minio=MagicMock(return_value={"status": health_module.STATUS_OK}),
        ):
            response = api_client.get("/api/v1/health/")
        assert response.status_code == 503
        data = response.json()
        assert data["status"] == "fail"
        assert data["checks"]["db"]["status"] == "fail"
        assert "connection refused" in data["checks"]["db"]["error"]

    def test_health_endpoint_skips_minio_when_not_configured(self, api_client):
        with patch.multiple(
            health_module,
            _check_db=MagicMock(return_value={"status": health_module.STATUS_OK}),
            _check_redis=MagicMock(return_value={"status": health_module.STATUS_OK}),
            _check_minio=MagicMock(
                return_value={"status": health_module.STATUS_SKIPPED, "reason": "MINIO_ENDPOINT vazio"}
            ),
        ):
            response = api_client.get("/api/v1/health/")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "ok"
        assert data["checks"]["minio"]["status"] == "skipped"

    def test_redis_fail_returns_503(self, api_client):
        with patch.multiple(
            health_module,
            _check_db=MagicMock(return_value={"status": health_module.STATUS_OK}),
            _check_redis=MagicMock(
                return_value={"status": health_module.STATUS_FAIL, "error": "broker down"}
            ),
            _check_minio=MagicMock(return_value={"status": health_module.STATUS_OK}),
        ):
            response = api_client.get("/api/v1/health/")
        assert response.status_code == 503
        assert response.json()["checks"]["redis"]["status"] == "fail"


# ─── Implementação dos checkers individuais ──────────────────────────────────


class TestCheckDB:
    def test_returns_ok_on_success(self):
        with patch.object(health_module, "connection") as mock_conn:
            mock_conn.ensure_connection = MagicMock()
            cursor_ctx = mock_conn.cursor.return_value.__enter__.return_value
            cursor_ctx.execute = MagicMock()
            cursor_ctx.fetchone = MagicMock(return_value=(1,))
            assert health_module._check_db()["status"] == "ok"

    def test_returns_fail_on_operational_error(self):
        from django.db import OperationalError

        with patch.object(health_module, "connection") as mock_conn:
            mock_conn.ensure_connection = MagicMock(
                side_effect=OperationalError("connection refused")
            )
            result = health_module._check_db()
            assert result["status"] == "fail"
            assert "connection refused" in result["error"]


class TestCheckRedis:
    def test_skipped_when_redis_url_empty(self):
        with patch.object(health_module.settings, "REDIS_URL", "", create=True):
            result = health_module._check_redis()
        assert result["status"] == "skipped"

    def test_fail_when_ping_raises(self):
        fake_redis = MagicMock()
        fake_client = MagicMock()
        fake_client.ping.side_effect = RuntimeError("broker down")
        fake_redis.from_url.return_value = fake_client
        with (
            patch.object(health_module.settings, "REDIS_URL", "redis://x:6379/0", create=True),
            patch.dict("sys.modules", {"redis": fake_redis}),
        ):
            result = health_module._check_redis()
        assert result["status"] == "fail"
        assert "broker down" in result["error"]


class TestCheckMinio:
    def test_skipped_when_endpoint_empty(self):
        with (
            patch.object(health_module.settings, "MINIO_ENDPOINT", "", create=True),
            patch.object(health_module.settings, "MINIO_BUCKET", "x", create=True),
        ):
            result = health_module._check_minio()
        assert result["status"] == "skipped"

    def test_skipped_when_boto3_not_installed(self):
        # Simula ImportError forçando o import a falhar via builtin.
        import builtins

        original_import = builtins.__import__

        def fake_import(name, *args, **kwargs):
            if name == "boto3":
                raise ImportError("No module named 'boto3'")
            return original_import(name, *args, **kwargs)

        with (
            patch.object(health_module.settings, "MINIO_ENDPOINT", "http://x:9000", create=True),
            patch.object(health_module.settings, "MINIO_BUCKET", "b", create=True),
            patch.object(builtins, "__import__", side_effect=fake_import),
        ):
            result = health_module._check_minio()
        assert result["status"] == "skipped"

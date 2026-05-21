"""Healthcheck rico em ``GET /api/v1/health/``.

Retorna um JSON plano com checks individuais (db/redis/minio) e status agregado.
Usado por probes de container (K8s liveness/readiness, docker healthcheck) e
ferramentas externas (Uptime Kuma).

Payload (JSON puro — **não** passa pelo ``EnvelopeRenderer`` pra que probes
externos consumam o shape canônico de healthcheck):

```json
{
  "status": "ok" | "fail",
  "checks": {
    "db":    {"status": "ok" | "fail", "error": "..."},
    "redis": {"status": "ok" | "fail" | "skipped", "reason": "..."},
    "minio": {"status": "ok" | "fail" | "skipped", "reason": "..."}
  }
}
```

Regras:

* ``db`` é obrigatório — sem Postgres o backend é inútil → 503 se falhar.
* ``redis`` é ``skipped`` quando ``REDIS_URL`` não estiver setado; só vira
  ``fail`` se a URL estiver populada mas o ping explodir.
* ``minio`` é ``skipped`` quando ``MINIO_ENDPOINT`` vazio ou ``boto3`` não
  está instalado (boto3 é opcional no template).
* Qualquer ``fail`` → ``status: "fail"`` agregado + HTTP 503. ``skipped``
  agregado segue ``ok`` (dep não configurada não derruba o probe).
"""
from typing import Any

from django.conf import settings
from django.db import OperationalError, connection
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_GET
from rest_framework import status as http_status

# Códigos canônicos. Mantidos como constantes pra testes/dashboards
# cruzarem sem magic strings.
STATUS_OK = "ok"
STATUS_FAIL = "fail"
STATUS_SKIPPED = "skipped"

AGGREGATE_OK = "ok"
AGGREGATE_FAIL = "fail"


def _ok() -> dict[str, str]:
    return {"status": STATUS_OK}


def _fail(error: str) -> dict[str, str]:
    return {"status": STATUS_FAIL, "error": error}


def _skipped(reason: str) -> dict[str, str]:
    return {"status": STATUS_SKIPPED, "reason": reason}


def _check_db() -> dict[str, str]:
    """Verifica Postgres via ``SELECT 1``."""
    try:
        connection.ensure_connection()
        with connection.cursor() as cursor:
            cursor.execute("SELECT 1")
            cursor.fetchone()
    except (OperationalError, Exception) as exc:  # noqa: BLE001
        return _fail(str(exc))
    return _ok()


def _check_redis() -> dict[str, str]:
    """Verifica Redis. ``skipped`` se ``REDIS_URL`` vazio.

    Usa ``redis.from_url(...).ping()``. Se o pacote ``redis`` não estiver
    instalado (improvável — está em deps base), também marca ``skipped``.
    """
    redis_url = getattr(settings, "REDIS_URL", "") or ""
    if not redis_url:
        return _skipped("REDIS_URL não configurado")

    try:
        import redis  # type: ignore[import-not-found]
    except ImportError:
        return _skipped("pacote `redis` não instalado")

    try:
        client = redis.from_url(redis_url, socket_connect_timeout=1, socket_timeout=1)
        client.ping()
    except Exception as exc:  # noqa: BLE001
        return _fail(str(exc))
    return _ok()


def _check_minio() -> dict[str, str]:
    """Verifica MinIO/S3 via ``head_bucket`` no bucket configurado.

    ``skipped`` quando:

    * ``MINIO_ENDPOINT`` vazio (deploy sem object storage), OU
    * ``boto3`` não está instalado (template não força a dep).
    """
    endpoint = getattr(settings, "MINIO_ENDPOINT", "") or ""
    bucket = getattr(settings, "MINIO_BUCKET", "") or ""
    if not endpoint or not bucket:
        return _skipped("MINIO_ENDPOINT/BUCKET não configurado")

    try:
        import boto3  # type: ignore[import-not-found]
        from botocore.client import Config  # type: ignore[import-not-found]
    except ImportError:
        return _skipped("`boto3` não instalado")

    try:
        client = boto3.client(
            "s3",
            endpoint_url=endpoint,
            aws_access_key_id=getattr(settings, "MINIO_ROOT_USER", ""),
            aws_secret_access_key=getattr(settings, "MINIO_ROOT_PASSWORD", ""),
            config=Config(connect_timeout=1, read_timeout=1, retries={"max_attempts": 1}),
        )
        client.head_bucket(Bucket=bucket)
    except Exception as exc:  # noqa: BLE001
        return _fail(str(exc))
    return _ok()


def _aggregate(checks: dict[str, dict[str, str]]) -> tuple[str, int]:
    """Decide status agregado + HTTP code.

    Qualquer ``fail`` → ``fail`` + 503. ``skipped`` puro segue ``ok`` + 200.
    """
    if any(c.get("status") == STATUS_FAIL for c in checks.values()):
        return AGGREGATE_FAIL, http_status.HTTP_503_SERVICE_UNAVAILABLE
    return AGGREGATE_OK, http_status.HTTP_200_OK


@csrf_exempt
@require_GET
def health_view(_request: Any) -> JsonResponse:
    """Healthcheck profundo — db + redis + minio.

    Público (sem auth). Retorna ``JsonResponse`` plano (sem envelope DRF) pra
    que probes/uptime checkers consumam o shape canônico. Para checks de
    serviços externos definidos em runtime, ver app ``healthcheck``.
    """
    checks: dict[str, dict[str, str]] = {
        "db": _check_db(),
        "redis": _check_redis(),
        "minio": _check_minio(),
    }
    aggregated, http_code = _aggregate(checks)
    return JsonResponse({"status": aggregated, "checks": checks}, status=http_code)


__all__ = [
    "AGGREGATE_FAIL",
    "AGGREGATE_OK",
    "STATUS_FAIL",
    "STATUS_OK",
    "STATUS_SKIPPED",
    "_check_db",
    "_check_minio",
    "_check_redis",
    "health_view",
]

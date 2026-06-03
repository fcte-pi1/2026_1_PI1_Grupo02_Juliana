"""Integração com Sentry — opt-in via ``SENTRY_DSN``.

Contrato:

* DSN vazio (default em dev/teste) → :func:`init_sentry` retorna ``False``
  e não invoca ``sentry_sdk.init``. Nenhum cliente é criado, nenhum HTTP é
  feito, nenhuma conta Sentry é exigida pra rodar local.
* DSN preenchido → inicializa com integrations Django + Celery + Logging.
  ``send_default_pii=False`` por padrão (LGPD): não enviamos IP, body de
  request, nem headers brutos. ``before_send`` redige headers sensíveis
  como defesa em profundidade.

Configurado em ``config/settings/base.py``. Vars consumidas:
``SENTRY_DSN``, ``SENTRY_ENVIRONMENT``, ``SENTRY_TRACES_SAMPLE_RATE``,
``SENTRY_RELEASE``.
"""
from typing import Any

import sentry_sdk
from sentry_sdk.integrations.celery import CeleryIntegration
from sentry_sdk.integrations.django import DjangoIntegration
from sentry_sdk.integrations.logging import LoggingIntegration

# Headers nunca enviados pro Sentry — redigidos em ``_before_send``.
_SENSITIVE_HEADERS = (
    "Authorization",
    "X-API-Key",
    "authorization",
    "x-api-key",
)


def _before_send(event: dict[str, Any], _hint: dict[str, Any]) -> dict[str, Any]:
    """Hook ``before_send`` do Sentry: redige headers sensíveis.

    Retornar ``None`` descartaria o evento; aqui só mutamos in-place.
    """
    request = event.get("request") or {}
    headers = request.get("headers") or {}
    if isinstance(headers, dict):
        for header_key in _SENSITIVE_HEADERS:
            if header_key in headers:
                headers[header_key] = "[REDACTED]"
    return event


def init_sentry(
    *,
    dsn: str,
    environment: str = "local",
    release: str | None = None,
    traces_sample_rate: float = 0.0,
) -> bool:
    """Inicializa o Sentry SDK se ``dsn`` estiver presente.

    Args:
        dsn: DSN do projeto Sentry. String vazia = no-op total.
        environment: nome do ambiente (``local``/``staging``/``production``).
        release: versão/sha do deploy. Opcional.
        traces_sample_rate: fração [0,1] de transações com trace.

    Returns:
        ``True`` se inicializou, ``False`` se virou no-op.
    """
    if not dsn:
        return False

    sentry_sdk.init(
        dsn=dsn,
        environment=environment,
        release=release,
        traces_sample_rate=traces_sample_rate,
        send_default_pii=False,
        before_send=_before_send,
        integrations=[
            DjangoIntegration(),
            CeleryIntegration(),
            LoggingIntegration(),
        ],
    )
    return True


__all__ = ["init_sentry"]

"""Helpers pra montar payloads que serão envolvidos pelo `EnvelopeRenderer`.

Use cases devolvem dicts plain; views chamam `Response(data)`. Estes helpers
são úteis quando uma view precisa devolver explicitamente um envelope montado
(raro — prefira deixar o renderer fazer o trabalho).
"""
from datetime import UTC, datetime


def build_envelope(data, request=None, pagination=None, warnings=None):
    """Monta o envelope canônico de sucesso."""
    return {
        "success": True,
        "data": data,
        "meta": {
            "timestamp": datetime.now(UTC).isoformat(),
            "request_id": getattr(request, "request_id", None) if request else None,
        },
        "pagination": pagination,
        "errors": None,
        "warnings": warnings,
    }


def error_envelope(code: str, message: str, fields: dict | None = None, request=None):
    """Monta o envelope canônico de erro."""
    return {
        "success": False,
        "data": None,
        "meta": {
            "timestamp": datetime.now(UTC).isoformat(),
            "request_id": getattr(request, "request_id", None) if request else None,
        },
        "pagination": None,
        "errors": [
            {
                "code": code,
                "message": message,
                "fields": fields or {},
            }
        ],
        "warnings": None,
    }

"""EnvelopeRenderer — toda response da API é envolvida no shape canônico:

```
{
  "success": bool,
  "data": <payload | null>,
  "meta": {"timestamp": iso, "request_id": str | null},
  "pagination": {count, page, page_size, total_pages, next, previous} | null,
  "errors": [{code, message, fields}] | null,
  "warnings": [...] | null
}
```

Regras:
- Status 204: corpo vazio (não envolve).
- 4xx/5xx: `success=false`, `data=null`, `errors` populado.
- Lista paginada da DRF (com `count`/`results`/`next`/`previous`): extrai `results`
  pra `data` e popula `pagination`.
- Sucesso simples: payload vai direto pra `data`.

Views devolvem `Response(data)` plain; este renderer encapsula.
"""
from datetime import UTC, datetime

from rest_framework.renderers import JSONRenderer


def _now_iso() -> str:
    return datetime.now(UTC).isoformat()


def _request_id(renderer_context) -> str | None:
    if not renderer_context:
        return None
    request = renderer_context.get("request")
    return getattr(request, "request_id", None) if request else None


def _is_paginated(payload) -> bool:
    return (
        isinstance(payload, dict)
        and "results" in payload
        and "count" in payload
        and "next" in payload
        and "previous" in payload
    )


def _is_error_payload(payload) -> bool:
    """Detecta o shape `{code, message, fields}` produzido pelo
    `custom_exception_handler` para erros de domínio.
    """
    return (
        isinstance(payload, dict)
        and "code" in payload
        and "message" in payload
        and set(payload.keys()) <= {"code", "message", "fields"}
    )


def _normalize_drf_error(payload):
    """DRF default error responses podem ser dict, list ou ReturnDict.
    Normaliza pra lista de `errors` no envelope.
    """
    if isinstance(payload, dict):
        if "detail" in payload:
            return [
                {
                    "code": "APIError",
                    "message": str(payload["detail"]),
                    "fields": {},
                }
            ]
        # Erros de validação: campo -> [mensagens]
        return [
            {
                "code": "ValidationError",
                "message": "Validation failed.",
                "fields": payload,
            }
        ]
    if isinstance(payload, list):
        return [
            {
                "code": "APIError",
                "message": str(payload),
                "fields": {},
            }
        ]
    return [
        {
            "code": "APIError",
            "message": str(payload),
            "fields": {},
        }
    ]


class EnvelopeRenderer(JSONRenderer):
    def render(self, data, accepted_media_type=None, renderer_context=None):
        renderer_context = renderer_context or {}
        response = renderer_context.get("response")
        status_code = getattr(response, "status_code", 200)

        # 204: corpo vazio
        if status_code == 204:
            return b""

        meta = {"timestamp": _now_iso(), "request_id": _request_id(renderer_context)}

        # Erros (4xx/5xx)
        if status_code >= 400:
            errors = [data] if _is_error_payload(data) else _normalize_drf_error(data)
            envelope = {
                "success": False,
                "data": None,
                "meta": meta,
                "pagination": None,
                "errors": errors,
                "warnings": None,
            }
            return super().render(envelope, accepted_media_type, renderer_context)

        # Sucesso paginado
        if _is_paginated(data):
            results = data["results"]
            count = data["count"]
            request = renderer_context.get("request")
            view = renderer_context.get("view")
            # page_size real do paginator (respeita ?page_size= e max_page_size).
            # Fallback: tamanho da página atual.
            page_size = len(results) or 1
            try:
                paginator = getattr(view, "paginator", None)
                if paginator is not None and request is not None:
                    page_size = paginator.get_page_size(request) or page_size
            except Exception:  # noqa: BLE001
                pass
            # tenta inferir page atual via query param
            page = 1
            if request is not None:
                try:
                    page = int(request.query_params.get("page", 1))
                except (TypeError, ValueError):
                    page = 1
            total_pages = (count + page_size - 1) // page_size if page_size else 1
            envelope = {
                "success": True,
                "data": results,
                "meta": meta,
                "pagination": {
                    "count": count,
                    "page": page,
                    "page_size": page_size,
                    "total_pages": total_pages,
                    "next": data["next"],
                    "previous": data["previous"],
                },
                "errors": None,
                "warnings": None,
            }
            return super().render(envelope, accepted_media_type, renderer_context)

        # Sucesso simples
        envelope = {
            "success": True,
            "data": data,
            "meta": meta,
            "pagination": None,
            "errors": None,
            "warnings": None,
        }
        return super().render(envelope, accepted_media_type, renderer_context)

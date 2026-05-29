"""Exception types e exception handler global da API.

Use cases / services lançam `ApplicationError` (ou subclasses). Nunca importar
DRF/HTTP nesses módulos — o handler aqui é o ponto de tradução.
"""
from django.http import Http404
from rest_framework.exceptions import NotFound
from rest_framework.response import Response
from rest_framework.views import exception_handler


class ApplicationError(Exception):
    """Erro de domínio. Subclasses redefinem `status_code`."""

    status_code = 400

    def __init__(self, message: str, *, extra: dict | None = None):
        super().__init__(message)
        self.message = message
        self.extra = extra or {}


class NotFoundError(ApplicationError):
    status_code = 404


class PermissionDeniedError(ApplicationError):
    status_code = 403


def custom_exception_handler(exc, context):
    """Mapeia erros de domínio + 404 não-tratado pro shape padrão da API.

    Shape devolvido aqui é apenas o "payload" — o `EnvelopeRenderer` envolve
    em `{success, data, errors, ...}`. Aqui só montamos o objeto que vai pra
    `errors` (e a view DRF coloca o status code adequado).
    """
    if isinstance(exc, ApplicationError):
        return Response(
            {
                "code": type(exc).__name__,
                "message": exc.message,
                "fields": exc.extra,
            },
            status=exc.status_code,
        )

    # Http404 do Django vira NotFoundError equivalente
    if isinstance(exc, Http404 | NotFound):
        return Response(
            {
                "code": "NotFoundError",
                "message": str(exc) or "Not found.",
                "fields": {},
            },
            status=404,
        )

    return exception_handler(exc, context)

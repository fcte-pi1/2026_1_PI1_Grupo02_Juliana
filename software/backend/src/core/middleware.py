"""RequestIDMiddleware — gera (ou propaga) X-Request-ID por request.

Se o cliente já enviou ``X-Request-ID``, respeitamos; senão, geramos um UUID v7
(ordenado cronologicamente — facilita debugging em logs). O id fica em
``request.request_id`` pra views/use cases logarem.

Também faz ``logger.contextualize(request_id=...)`` em volta do request:
todo log emitido durante o ciclo carrega o ``request_id`` no extra, que o
formatter pretty/JSON consome via ``{extra[request_id]}``.
"""
import uuid
from collections.abc import Callable

from django.http import HttpRequest, HttpResponse
from loguru import logger

try:
    _new_id = uuid.uuid7  # Python 3.14+
except AttributeError:  # pragma: no cover
    _new_id = uuid.uuid4


class RequestIDMiddleware:
    HEADER = "X-Request-ID"
    META_KEY = "HTTP_X_REQUEST_ID"

    def __init__(self, get_response: Callable[[HttpRequest], HttpResponse]):
        self.get_response = get_response

    def __call__(self, request: HttpRequest) -> HttpResponse:
        incoming = request.META.get(self.META_KEY)
        request_id = incoming or str(_new_id())
        request.request_id = request_id

        with logger.contextualize(request_id=request_id):
            response = self.get_response(request)
        response[self.HEADER] = request_id
        return response

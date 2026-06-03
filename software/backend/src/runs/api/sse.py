"""Endpoint SSE de telemetria — view Django plain (fora do DRF).

Fora do DRF de propósito: o `EnvelopeRenderer` é JSON; SSE precisa de
`text/event-stream` cru. Faz streaming do canal Redis alimentado pelo subscriber
MQTT (ver `runs/realtime.py`).

Auth: `EventSource` não manda header `Authorization`, então o JWT vem por query
param `?token=<access>` e é validado manualmente.
"""
from __future__ import annotations

import json

from django.http import HttpResponse, HttpResponseForbidden, StreamingHttpResponse
from rest_framework_simplejwt.exceptions import TokenError
from rest_framework_simplejwt.tokens import AccessToken

from core.errors import NotFoundError
from runs.realtime import subscribe
from runs.selectors import get_tentativa_by_id
from runs.services.snapshot import build_snapshot


def _sse(data: dict) -> str:
    return f"data: {json.dumps(data, default=str)}\n\n"


def _event_stream(tentativa_id: str):
    # Snapshot inicial: o cliente já desenha o estado atual sem esperar o 1º tick.
    tentativa = get_tentativa_by_id(tentativa_id=tentativa_id)
    yield _sse(build_snapshot(tentativa))

    pubsub = subscribe(tentativa_id)
    try:
        while True:
            msg = pubsub.get_message(timeout=5.0)
            if msg and msg.get("type") == "message":
                yield f"data: {msg['data'].decode()}\n\n"
            else:
                # Comentário SSE: keepalive + detecta desconexão do cliente.
                yield ": keepalive\n\n"
    finally:
        pubsub.close()


def telemetry_stream(request, pk: str):
    token = request.GET.get("token", "")
    try:
        AccessToken(token)
    except TokenError:
        return HttpResponseForbidden("invalid or missing token")

    try:
        get_tentativa_by_id(tentativa_id=pk)
    except NotFoundError:
        return HttpResponse(status=404)

    response = StreamingHttpResponse(
        _event_stream(pk), content_type="text/event-stream"
    )
    response["Cache-Control"] = "no-cache"
    response["X-Accel-Buffering"] = "no"  # desliga buffering em proxies (nginx)
    return response

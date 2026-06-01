"""Ponte de tempo real entre o processo subscriber MQTT e as conexões SSE.

O subscriber e o web server são processos separados; o Redis (já no stack) faz
o fan-out. O subscriber publica o snapshot no canal `telemetry:<tentativa_id>`;
cada view SSE assina esse canal e repassa ao browser.
"""
from __future__ import annotations

import json

import redis
from django.conf import settings


def _client() -> redis.Redis:
    return redis.Redis.from_url(settings.REDIS_URL)


def channel(tentativa_id: str) -> str:
    return f"telemetry:{tentativa_id}"


def publish_snapshot(tentativa_id: str, snapshot: dict) -> None:
    _client().publish(channel(tentativa_id), json.dumps(snapshot, default=str))


def subscribe(tentativa_id: str):
    """Retorna um pubsub já inscrito no canal da tentativa. O chamador itera
    em `listen()` e fecha no final.
    """
    pubsub = _client().pubsub()
    pubsub.subscribe(channel(tentativa_id))
    return pubsub

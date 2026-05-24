"""Tasks Celery do app runs.

`simular_corrida` faz o papel do robô para o botão "Iniciar run": roda a
simulação publicando telemetria no MQTT (o subscriber persiste e empurra via SSE).
O botão "Parar run" seta uma flag no Redis que a simulação consulta a cada passo.
"""
from __future__ import annotations

import redis
from celery import shared_task
from django.conf import settings

from runs.models import Tentativa
from runs.services.simulator import run_simulation


def _stop_key(run_id: str) -> str:
    return f"sim:stop:{run_id}"


def _redis() -> redis.Redis:
    return redis.Redis.from_url(settings.REDIS_URL)


def parar_corrida(tentativa_id: str) -> None:
    """Sinaliza para a simulação em andamento abortar (consumido por `should_stop`)."""
    _redis().set(_stop_key(tentativa_id), "1", ex=600)


@shared_task
def simular_corrida(tentativa_id: str, hz: float = 2.0, steps: int = 48) -> bool:
    r = _redis()
    r.delete(_stop_key(tentativa_id))  # limpa flag de parada anterior
    tentativa = Tentativa.objects.select_related("labirinto", "micromouse").get(
        id=tentativa_id
    )
    if tentativa.status == Tentativa.Status.FINALIZADA:
        tentativa.maze = {}
        tentativa.save(update_fields=["maze", "updated_at"])  # recomeça o mapa
    completed = run_simulation(
        tentativa=tentativa,
        hz=hz,
        steps=steps,
        should_stop=lambda: bool(r.exists(_stop_key(tentativa_id))),
    )
    if not completed:
        tentativa.refresh_from_db()
        tentativa.status = Tentativa.Status.ABORTADA
        tentativa.save(update_fields=["status", "updated_at"])
    return completed

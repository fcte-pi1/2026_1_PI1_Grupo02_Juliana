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
from runs.use_cases.persistir_telemetria import PersistirTelemetria
from runs import realtime
from asgiref.sync import async_to_sync
from channels.layers import get_channel_layer


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


@shared_task(bind=True)
def processar_telemetria(self, payload: dict) -> dict:
    """Celery task: process MQTT payload and persist via use case.

    Responsibilities:
    - receive payload (dict)
    - call the use case PersistirTelemetria
    - publish snapshot to Redis (SSE bridge)
    - forward a minimal update to the Channels group `telemetry` (WebSocket)
    """
    try:
        usecase = PersistirTelemetria()
        snapshot = usecase.execute(payload=payload)

        # Publish snapshot to Redis channel for SSE consumers
        tentativa_id = snapshot.get("tentativa_id")
        if tentativa_id:
            realtime.publish_snapshot(tentativa_id, snapshot)

        # Send minimal event to Channels group for WebSocket clients
        channel_layer = get_channel_layer()
        event = {
            "type": "telemetry.update",
            "robot_id": payload.get("robot_id") or payload.get("robotId"),
            "x": payload.get("position", {}).get("x") if isinstance(payload.get("position"), dict) else None,
            "y": payload.get("position", {}).get("y") if isinstance(payload.get("position"), dict) else None,
            "orientation": payload.get("orientation"),
            "velocity": payload.get("velocity") or payload.get("speed"),
            "battery": payload.get("battery"),
            "step": None,
        }
        # try to extract step if available in snapshot or payload
        try:
            if isinstance(snapshot, dict):
                # snapshot may include ts or other markers; extract passo from last pos
                pass
        except Exception:
            pass

        if channel_layer is not None:
            async_to_sync(channel_layer.group_send)("telemetry", event)

        return {"success": True, "snapshot": snapshot}
    except Exception as exc:  # pragma: no cover - runtime errors handled in worker
        # Log and re-raise so Celery records the failure
        from loguru import logger

        logger.exception("Error processing telemetry: {}", exc)
        raise

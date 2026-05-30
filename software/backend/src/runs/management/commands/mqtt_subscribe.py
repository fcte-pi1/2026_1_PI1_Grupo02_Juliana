"""Processo subscriber MQTT do backend.

Roda como serviço próprio (ver `docker/compose.yml`). Assina os tópicos de
telemetria/evento de todos os robôs, despacha pros use cases e republica o
snapshot resultante no Redis pra alimentar os clientes SSE.

> Decisão: subscriber como management command (processo dedicado), não Celery
> task — um loop MQTT persistente não encaixa no modelo de task do Celery.
"""
from __future__ import annotations

import json

from django.conf import settings
from django.core.management.base import BaseCommand
from loguru import logger

from integrations.mqtt.client import build_client
from runs import realtime
from runs.use_cases.registrar_evento import RegistrarEvento
from runs.tasks import processar_telemetria


class Command(BaseCommand):
    help = "Assina os tópicos MQTT do micromouse e persiste a telemetria."

    def handle(self, *args, **options):
        base = settings.MQTT_BASE_TOPIC
        client = build_client(client_id="backend-subscriber")
        client.on_connect = self._on_connect
        client.on_message = self._on_message
        logger.info("MQTT subscriber conectado em {}:{}", settings.MQTT_HOST, settings.MQTT_PORT)
        client.subscribe([(f"{base}/+/telemetria", 1), (f"{base}/+/evento", 1)])
        client.loop_forever()

    @staticmethod
    def _on_connect(client, userdata, flags, reason_code, properties):
        base = settings.MQTT_BASE_TOPIC
        client.subscribe([(f"{base}/+/telemetria", 1), (f"{base}/+/evento", 1)])

    @staticmethod
    def _on_message(client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
        except (ValueError, UnicodeDecodeError):
            logger.warning("Payload MQTT inválido em {}", msg.topic)
            return

        try:
            if msg.topic.endswith("/telemetria"):
                # Do not access DB here; enqueue Celery task to process telemetry
                processar_telemetria.delay(payload)
            elif msg.topic.endswith("/evento"):
                # events still handled synchronously via use case for now
                snapshot = RegistrarEvento().execute(payload=payload)
                realtime.publish_snapshot(snapshot["tentativa_id"], snapshot)
            else:
                return
        except Exception as exc:  # noqa: BLE001 — não derrubar o loop por 1 pacote ruim
            logger.error("Falha processando {}: {}", msg.topic, exc)

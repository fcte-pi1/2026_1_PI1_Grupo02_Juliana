"""Wrapper fino sobre paho-mqtt.

Espelha o estilo de `integrations/storage/s3_client.py`: config via Django
settings, sem regra de negócio aqui. Quem consome:
- `runs/management/commands/mqtt_subscribe.py` (subscriber do backend);
- `runs/management/commands/mqtt_simulate.py` (publisher de teste);
- `runs/use_cases` ao publicar comandos (start/stop) para o robô.

Tópicos (definidos em `docs/.../06_projetoconceitual.tex`):
- `<base>/<id>/telemetria` — QoS 1, não-retained, 10–30 Hz (pose, maze_delta, ...)
- `<base>/<id>/evento`     — QoS 1, retained (inicio/colisao/reparo/desafio_cumprido)
- `<base>/<id>/status`     — QoS 1, retained (online/offline, via LWT)
"""
from __future__ import annotations

import paho.mqtt.client as mqtt
from django.conf import settings


def topic_telemetria(micromouse_id: str) -> str:
    return f"{settings.MQTT_BASE_TOPIC}/{micromouse_id}/telemetria"


def topic_evento(micromouse_id: str) -> str:
    return f"{settings.MQTT_BASE_TOPIC}/{micromouse_id}/evento"


def topic_status(micromouse_id: str) -> str:
    return f"{settings.MQTT_BASE_TOPIC}/{micromouse_id}/status"


def topic_comando(micromouse_id: str) -> str:
    return f"{settings.MQTT_BASE_TOPIC}/{micromouse_id}/comando"


def build_client(*, client_id: str = "") -> mqtt.Client:
    """Cria um client paho conectado, com auth opcional. Não inicia o loop —
    o chamador decide entre `loop_forever()` (subscriber) e `publish()` pontual.
    """
    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id=client_id,
    )
    if settings.MQTT_USERNAME:
        client.username_pw_set(settings.MQTT_USERNAME, settings.MQTT_PASSWORD)
    client.connect(settings.MQTT_HOST, settings.MQTT_PORT, keepalive=60)
    return client

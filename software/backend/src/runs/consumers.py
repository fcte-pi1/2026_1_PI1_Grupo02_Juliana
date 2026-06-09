from __future__ import annotations

import json
from channels.generic.websocket import AsyncWebsocketConsumer


class TelemetryConsumer(AsyncWebsocketConsumer):
    """WebSocket consumer that subscribes clients to the `telemetry` group.

    Sends telemetry updates published to the `telemetry` group.
    """

    async def connect(self):
        await self.channel_layer.group_add("telemetry", self.channel_name)
        await self.accept()

    async def disconnect(self, close_code):
        await self.channel_layer.group_discard("telemetry", self.channel_name)

    async def telemetry_update(self, event: dict):
        # event is a dict with telemetry fields; forward to websocket client
        await self.send(text_data=json.dumps(event))

    async def receive(self, text_data=None, bytes_data=None):
        # no-op: clients don't send messages in this protocol
        return

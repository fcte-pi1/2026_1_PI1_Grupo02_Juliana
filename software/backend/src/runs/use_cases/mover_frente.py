"""RF01 — Movimentação frontal: publica comando FRENTE para o firmware via MQTT."""
from __future__ import annotations

from integrations.mqtt.client import build_client, topic_comando
from runs.selectors import get_tentativa_by_id


class MoverFrente:
    """Publica 'FRENTE <velocidade>' no tópico de comando do firmware.

    O firmware aplica PID de heading para manter linha reta (RF01):
    - desvio ≤ 1 cm/m garantido pelo PID de correção de heading
    - velocidade constante sem paradas não programadas
    - parada em ≤ 2 cm ao receber o comando STOP (PWM zerado imediatamente)
    """

    VELOCIDADE_PADRAO = 150
    VELOCIDADE_MAX = 255

    def execute(self, tentativa_id: str, velocidade: int | None = None) -> dict:
        vel = int(self.VELOCIDADE_PADRAO if velocidade is None else velocidade)
        vel = max(1, min(self.VELOCIDADE_MAX, vel))
        tentativa = get_tentativa_by_id(tentativa_id=tentativa_id)
        client = build_client(client_id=f"rf01-{tentativa_id}")
        client.publish(topic_comando(str(tentativa.id)), f"FRENTE {vel}", qos=1)
        client.disconnect()
        return {"acao": "mover_frente", "velocidade": vel}

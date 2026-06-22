"""RF02 — Rotação: publica comando GIRAR para o firmware via MQTT."""
from __future__ import annotations

from integrations.mqtt.client import build_client, topic_comando
from runs.selectors import get_tentativa_by_id

ANGULOS_VALIDOS = frozenset({90, -90, 180})


class Girar:
    """Publica 'GIRAR <angulo>' no tópico de comando do firmware.

    RF02 — critérios atendidos pelo firmware:
    - Giro com erro ≤ ±2° (calibrado via MS_POR_GRAU na classe Rotacao)
    - Permanece na mesma célula do labirinto (motores opostos, giro no eixo)
    - Orientação interna atualizada ao concluir, pronta para o próximo movimento
    """

    ANGULOS_VALIDOS = ANGULOS_VALIDOS

    def execute(self, tentativa_id: str, angulo: int) -> dict:
        if angulo not in self.ANGULOS_VALIDOS:
            raise ValueError(f"Ângulo inválido: {angulo}. Use 90, -90 ou 180.")
        tentativa = get_tentativa_by_id(tentativa_id=tentativa_id)
        client = build_client(client_id=f"rf02-{tentativa_id}")
        client.publish(topic_comando(str(tentativa.id)), f"GIRAR {angulo}", qos=1)
        client.disconnect()
        return {"acao": "girar", "angulo": angulo}

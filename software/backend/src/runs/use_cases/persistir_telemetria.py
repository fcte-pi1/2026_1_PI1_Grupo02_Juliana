"""Use case `PersistirTelemetria` — aplica um pacote de telemetria à tentativa.

Recebe o payload cru (dict) do subscriber MQTT, valida, aplica o `maze_delta` ao
estado autoritativo do labirinto, registra a posição na trajetória e devolve o
snapshot pronto pro dashboard. Persistência aqui; render no frontend.
"""
from __future__ import annotations

from runs.models import Posicao, Tentativa
from runs.schemas import TelemetriaPayload
from runs.selectors import get_tentativa_by_id
from runs.services.maze import apply_delta
from runs.services.snapshot import build_snapshot


class PersistirTelemetria:
    def execute(self, *, payload: dict) -> dict:
        data = TelemetriaPayload.model_validate(payload)
        tentativa = get_tentativa_by_id(tentativa_id=data.run_id)
        # Apply maze delta and update pose
        tentativa.maze = apply_delta(tentativa.maze, data.maze_delta)
        tentativa.pose = data.pose.model_dump()

        # Compute next step index
        current_count = tentativa.posicoes.count()
        next_step = current_count + 1

        # Update aggregate metrics
        if data.speed is not None:
            if tentativa.velocidade_media is None:
                tentativa.velocidade_media = data.speed
            else:
                tentativa.velocidade_media = (
                    (tentativa.velocidade_media * current_count + data.speed) / (next_step)
                )
        if data.battery is not None:
            tentativa.consumo_bateria = data.battery
        if tentativa.status != Tentativa.Status.EM_CURSO:
            tentativa.status = Tentativa.Status.EM_CURSO

        tentativa.save(
            update_fields=[
                "maze", "pose", "velocidade_media", "consumo_bateria",
                "status", "updated_at",
            ]
        )

        # Persist position
        Posicao.objects.create(
            tentativa=tentativa,
            coordenada_x=data.pose.x,
            coordenada_y=data.pose.y,
            timestamp=data.ts,
            passo=next_step,
            orientacao=data.pose.heading,
            velocidade=data.speed,
            bateria=data.battery,
        )

        return build_snapshot(
            tentativa,
            speed=data.speed,
            battery=data.battery,
            voltage=data.voltage,
            ts=data.ts,
        )

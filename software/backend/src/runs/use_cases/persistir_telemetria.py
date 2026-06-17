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
        
        # 1. Aplica o delta do labirinto e atualiza a pose
        tentativa.maze = apply_delta(tentativa.maze, data.maze_delta)
        tentativa.pose = data.pose.model_dump()

        # 2. Recupera a última posição para cálculos de tempo
        ultima_posicao = tentativa.posicoes.last()
        next_step = (ultima_posicao.passo + 1) if ultima_posicao else 1

        # 3. Atualização das Métricas (Cálculo Físico da RF09)
        if not tentativa.tempo_inicio:
            # Primeiro pacote da corrida: inicializa os cronômetros
            tentativa.tempo_inicio = data.ts
            tentativa.velocidade_media = data.speed if data.speed is not None else 0.0
        else:
            if ultima_posicao and data.speed is not None:
                t_prev = ultima_posicao.timestamp
                t_current = data.ts
                
                # Tempo do trecho atual (Delta t) e Tempo Total
                delta_t = (t_current - t_prev).total_seconds()
                total_t = (t_current - tentativa.tempo_inicio).total_seconds()

                if total_t > 0:
                    # Descompacta a distância anterior: D = v_m * t_total_anterior
                    t_prev_total = (t_prev - tentativa.tempo_inicio).total_seconds()
                    vm_anterior = tentativa.velocidade_media or 0.0
                    distancia_anterior = vm_anterior * t_prev_total

                    # Calcula a distância deste novo trecho: d = v * t
                    # Se speed for 0 (parado), distancia_trecho é 0, 
                    # mas o total_t continua crescendo, puxando a média para baixo.
                    distancia_trecho = data.speed * delta_t

                    # Nova Velocidade Média: Distância Total / Tempo Total
                    distancia_total = distancia_anterior + distancia_trecho
                    tentativa.velocidade_media = round(distancia_total / total_t, 4)

        if data.battery is not None:
            tentativa.consumo_bateria = data.battery
            
        if tentativa.status != Tentativa.Status.EM_CURSO:
            tentativa.status = Tentativa.Status.EM_CURSO

        # IMPORTANTE: "tempo_inicio" adicionado aos update_fields
        tentativa.save(
            update_fields=[
                "maze", "pose", "velocidade_media", "consumo_bateria",
                "status", "updated_at", "tempo_inicio"
            ]
        )

        # 4. Persiste a nova posição
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
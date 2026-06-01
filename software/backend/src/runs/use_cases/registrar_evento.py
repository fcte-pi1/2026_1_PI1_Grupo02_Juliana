"""Use case `RegistrarEvento` — trata eventos de ciclo de vida da corrida.

`inicio` marca o começo; `desafio_cumprido` fecha a tentativa com sucesso;
`colisao`/`reparo` apenas registram (poderiam pausar/retomar no futuro).
"""
from __future__ import annotations

from django.utils import timezone

from runs.models import Tentativa
from runs.schemas import EventoPayload
from runs.selectors import get_tentativa_by_id
from runs.services.snapshot import build_snapshot


class RegistrarEvento:
    def execute(self, *, payload: dict) -> dict:
        data = EventoPayload.model_validate(payload)
        tentativa = get_tentativa_by_id(tentativa_id=data.run_id)

        if data.type == "inicio":
            tentativa.tempo_inicio = data.ts
            tentativa.status = Tentativa.Status.EM_CURSO
        elif data.type == "desafio_cumprido":
            tentativa.tempo_fim = data.ts
            tentativa.sucesso = True
            tentativa.status = Tentativa.Status.FINALIZADA

        tentativa.save(
            update_fields=[
                "tempo_inicio", "tempo_fim", "sucesso", "status", "updated_at",
            ]
        )
        return build_snapshot(tentativa, ts=data.ts or timezone.now())

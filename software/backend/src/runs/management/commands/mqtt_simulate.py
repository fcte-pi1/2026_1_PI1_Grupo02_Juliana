"""Simulador de firmware via CLI — publica telemetria MQTT como o robô faria.

Stand-in do Raspberry Pi Pico W para testar o pipeline ponta a ponta sem hardware.
A lógica de fato vive em `runs/services/simulator.py` (compartilhada com a task
Celery acionada pelo botão "Iniciar run").

Uso:
    uv run python manage.py mqtt_simulate                 # cria tudo e simula
    uv run python manage.py mqtt_simulate --tentativa <id> # reusa uma existente
"""
from __future__ import annotations

from django.core.management.base import BaseCommand

from runs.models import Labirinto, Micromouse, Tentativa
from runs.services.simulator import run_simulation


class Command(BaseCommand):
    help = "Publica telemetria simulada no MQTT (stand-in do firmware)."

    def add_arguments(self, parser):
        parser.add_argument("--tentativa", default=None, help="ID de uma Tentativa existente.")
        parser.add_argument("--hz", type=float, default=2.0, help="Pacotes por segundo.")
        parser.add_argument("--steps", type=int, default=48, help="Máximo de células a percorrer.")

    def handle(self, *args, **options):
        tentativa = self._get_or_create_tentativa(options["tentativa"])
        n = tentativa.labirinto.dimensao
        self.stdout.write(
            self.style.SUCCESS(f"Simulando tentativa {tentativa.id} ({n}x{n})")
        )
        run_simulation(
            tentativa=tentativa,
            hz=options["hz"],
            steps=options["steps"],
            log=lambda msg: self.stdout.write(f"  → {msg}"),
        )
        self.stdout.write(self.style.SUCCESS("Simulação concluída."))

    def _get_or_create_tentativa(self, tentativa_id: str | None) -> Tentativa:
        if tentativa_id:
            return Tentativa.objects.select_related("labirinto", "micromouse").get(id=tentativa_id)
        mm, _ = Micromouse.objects.get_or_create(nome="Mouse-Sim", defaults={"algoritmo": "Flood Fill"})
        lab, _ = Labirinto.objects.get_or_create(nome="Labirinto-Sim", defaults={"dimensao": 16})
        tentativa = Tentativa.objects.create(micromouse=mm, labirinto=lab)
        self.stdout.write(
            self.style.WARNING(
                f"Tentativa criada: {tentativa.id}\n"
                f"  Dashboard: http://localhost:5173/?run={tentativa.id}"
            )
        )
        return tentativa

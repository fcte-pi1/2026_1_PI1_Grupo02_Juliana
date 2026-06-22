"""Simulador de firmware via CLI — publica telemetria MQTT como o robô faria.

Stand-in do Raspberry Pi Pico W para testar o pipeline ponta a ponta sem hardware.
A lógica de fato vive em `runs/services/simulator.py` (compartilhada com a task
Celery acionada pelo botão "Iniciar run").

Uso:
    uv run python manage.py mqtt_simulate                        # DFS 16×16, padrão
    uv run python manage.py mqtt_simulate --mode boustrophedon  # ziguezague legado
    uv run python manage.py mqtt_simulate --maze-size 8         # labirinto 8×8
    uv run python manage.py mqtt_simulate --speed 0.5           # robô mais rápido
    uv run python manage.py mqtt_simulate --battery-start 60    # começa com 60 %
    uv run python manage.py mqtt_simulate --tentativa <id>      # reusa existente
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
        parser.add_argument("--steps", type=int, default=120, help="Máximo de passos a simular.")
        parser.add_argument(
            "--mode",
            choices=["dfs", "boustrophedon"],
            default="dfs",
            help="Algoritmo de exploração: dfs (padrão, realista) ou boustrophedon (legado).",
        )
        parser.add_argument(
            "--speed",
            type=float,
            default=0.30,
            dest="base_speed",
            help="Velocidade base do robô em m/s (padrão: 0.30).",
        )
        parser.add_argument(
            "--battery-start",
            type=float,
            default=100.0,
            help="Nível inicial de bateria em %% (padrão: 100.0).",
        )
        parser.add_argument(
            "--maze-size",
            type=int,
            default=None,
            help="Dimensão do labirinto ao criar novo (padrão: 16). Ignorado com --tentativa.",
        )

    def handle(self, *args, **options):
        tentativa = self._get_or_create_tentativa(options["tentativa"], options["maze_size"])
        n = tentativa.labirinto.dimensao
        self.stdout.write(
            self.style.SUCCESS(
                f"Simulando tentativa {tentativa.id} ({n}x{n}) "
                f"mode={options['mode']} speed={options['base_speed']} m/s "
                f"bat_start={options['battery_start']}%%"
            )
        )
        run_simulation(
            tentativa=tentativa,
            hz=options["hz"],
            steps=options["steps"],
            mode=options["mode"],
            base_speed=options["base_speed"],
            battery_start=options["battery_start"],
            log=lambda msg: self.stdout.write(f"  → {msg}"),
        )
        self.stdout.write(self.style.SUCCESS("Simulação concluída."))

    def _get_or_create_tentativa(self, tentativa_id: str | None, maze_size: int | None) -> Tentativa:
        if tentativa_id:
            return Tentativa.objects.select_related("labirinto", "micromouse").get(id=tentativa_id)
        mm, _ = Micromouse.objects.get_or_create(nome="Mouse-Sim", defaults={"algoritmo": "Flood Fill"})
        dim = maze_size or 16
        lab, _ = Labirinto.objects.get_or_create(
            nome=f"Labirinto-Sim-{dim}x{dim}", defaults={"dimensao": dim}
        )
        tentativa = Tentativa.objects.create(micromouse=mm, labirinto=lab)
        self.stdout.write(
            self.style.WARNING(
                f"Tentativa criada: {tentativa.id}\n"
                f"  Dashboard: http://localhost:5173/?run={tentativa.id}"
            )
        )
        return tentativa

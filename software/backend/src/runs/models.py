"""Modelos de domínio das corridas do micromouse.

Alinhados ao MER de `docs/.../06_projetoconceitual.tex` (MICROMOUSE, LABIRINTO,
TENTATIVA, POSICAO). Extensão consciente ao MER: `Tentativa.maze` guarda a matriz
N×N de paredes descobertas (estado autoritativo montado a partir dos `maze_delta`
recebidos por MQTT) — o MER só modelava a trajetória (POSICAO).

Sem regra de negócio aqui: montagem do maze e métricas ficam nos use cases.
"""
from django.core.validators import MaxValueValidator, MinValueValidator
from django.db import models

from core.models.base import BaseModel


class Micromouse(BaseModel):
    nome = models.CharField(max_length=120)
    algoritmo = models.CharField(max_length=120, blank=True, default="")

    class Meta:
        ordering = ["-id"]

    def __str__(self) -> str:
        return self.nome


class Labirinto(BaseModel):
    nome = models.CharField(max_length=120)
    # Lado do grid quadrado (4, 8, 16...). Governa a renderização N×N no dashboard.
    dimensao = models.PositiveSmallIntegerField(
        default=16,
        validators=[MinValueValidator(2), MaxValueValidator(32)],
    )

    class Meta:
        ordering = ["-id"]

    def __str__(self) -> str:
        return f"{self.nome} ({self.dimensao}x{self.dimensao})"


class Tentativa(BaseModel):
    """Uma execução (corrida) de um micromouse num labirinto."""

    class Status(models.TextChoices):
        EM_CURSO = "em_curso", "Em curso"
        FINALIZADA = "finalizada", "Finalizada"
        ABORTADA = "abortada", "Abortada"

    micromouse = models.ForeignKey(
        Micromouse, on_delete=models.CASCADE, related_name="tentativas"
    )
    labirinto = models.ForeignKey(
        Labirinto, on_delete=models.PROTECT, related_name="tentativas"
    )
    tempo_inicio = models.DateTimeField(null=True, blank=True)
    tempo_fim = models.DateTimeField(null=True, blank=True)
    consumo_bateria = models.FloatField(null=True, blank=True)
    velocidade_media = models.FloatField(null=True, blank=True)
    sucesso = models.BooleanField(null=True, blank=True)
    status = models.CharField(
        max_length=12, choices=Status.choices, default=Status.EM_CURSO
    )
    # Estado autoritativo do mapa: dict {"<x>,<y>": {"n":bool,"s":bool,"e":bool,"w":bool}}.
    # Esparso — só células já descobertas aparecem. Montado pelos deltas (use case).
    maze = models.JSONField(default=dict, blank=True)
    # Última pose conhecida do robô: {"x":int,"y":int,"heading":"N|S|E|W"}.
    pose = models.JSONField(default=dict, blank=True)

    class Meta:
        ordering = ["-id"]

    def __str__(self) -> str:
        return f"Tentativa {self.id} — {self.micromouse} @ {self.labirinto}"


class Posicao(BaseModel):
    """Ponto da trajetória percorrida (um por passo persistido)."""

    tentativa = models.ForeignKey(
        Tentativa, on_delete=models.CASCADE, related_name="posicoes"
    )
    coordenada_x = models.IntegerField()
    coordenada_y = models.IntegerField()
    timestamp = models.DateTimeField()
    passo = models.PositiveIntegerField(default=0)
    orientacao = models.CharField(max_length=4, blank=True, default="")
    velocidade = models.FloatField(null=True, blank=True)
    bateria = models.FloatField(null=True, blank=True)

    class Meta:
        ordering = ["passo"]

    def __str__(self) -> str:
        return f"({self.coordenada_x},{self.coordenada_y}) passo={self.passo}"

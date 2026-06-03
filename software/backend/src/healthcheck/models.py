"""Modelo `ServiceCheck` — endpoint HTTP monitorado.

Fields cobrem cadastro (name, url, expected_status, interval_seconds,
is_active) + estado da última run (last_checked_at, last_status,
last_status_code, last_error). Sem método de domínio aqui — lógica em
selector ou use case.
"""
from django.core.validators import MaxValueValidator, MinValueValidator
from django.db import models

from core.models.base import BaseModel


class ServiceCheck(BaseModel):
    class Status(models.TextChoices):
        UNKNOWN = "unknown", "Desconhecido"
        OK = "ok", "OK"
        FAIL = "fail", "Falhou"

    name = models.CharField(max_length=120)
    url = models.URLField()
    expected_status = models.PositiveSmallIntegerField(
        default=200,
        validators=[MinValueValidator(100), MaxValueValidator(599)],
    )
    interval_seconds = models.PositiveIntegerField(default=300)
    is_active = models.BooleanField(default=True)
    last_checked_at = models.DateTimeField(null=True, blank=True)
    last_status = models.CharField(
        max_length=10,
        choices=Status.choices,
        default=Status.UNKNOWN,
    )
    last_status_code = models.PositiveSmallIntegerField(null=True, blank=True)
    last_error = models.TextField(blank=True, default="")

    class Meta:
        ordering = ["-id"]

    def __str__(self) -> str:
        return f"{self.name} ({self.url})"

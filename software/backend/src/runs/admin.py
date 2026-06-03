from django.contrib import admin

from runs.models import Labirinto, Micromouse, Posicao, Tentativa


@admin.register(Micromouse)
class MicromouseAdmin(admin.ModelAdmin):
    list_display = ("nome", "algoritmo", "created_at")
    search_fields = ("nome", "algoritmo")


@admin.register(Labirinto)
class LabirintoAdmin(admin.ModelAdmin):
    list_display = ("nome", "dimensao", "created_at")
    search_fields = ("nome",)


@admin.register(Tentativa)
class TentativaAdmin(admin.ModelAdmin):
    list_display = (
        "id",
        "micromouse",
        "labirinto",
        "status",
        "sucesso",
        "tempo_inicio",
        "tempo_fim",
    )
    list_filter = ("status", "sucesso")
    raw_id_fields = ("micromouse", "labirinto")


@admin.register(Posicao)
class PosicaoAdmin(admin.ModelAdmin):
    list_display = ("tentativa", "coordenada_x", "coordenada_y", "passo", "timestamp")
    raw_id_fields = ("tentativa",)

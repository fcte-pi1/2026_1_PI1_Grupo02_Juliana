from rest_framework import serializers

from runs.models import Labirinto, Micromouse, Tentativa


class MicromouseSerializer(serializers.ModelSerializer):
    class Meta:
        model = Micromouse
        fields = "__all__"
        read_only_fields = ("id", "created_at", "updated_at")


class LabirintoSerializer(serializers.ModelSerializer):
    class Meta:
        model = Labirinto
        fields = "__all__"
        read_only_fields = ("id", "created_at", "updated_at")


class TentativaSerializer(serializers.ModelSerializer):
    dimensao = serializers.IntegerField(source="labirinto.dimensao", read_only=True)
    explored = serializers.SerializerMethodField()

    class Meta:
        model = Tentativa
        fields = "__all__"
        read_only_fields = (
            "id", "created_at", "updated_at", "maze", "pose",
            "velocidade_media", "consumo_bateria", "sucesso", "status",
            "tempo_inicio", "tempo_fim",
        )

    def get_explored(self, obj) -> int:
        return len(obj.maze or {})


class PosicaoSerializer(serializers.ModelSerializer):
    class Meta:
        model = __import__("runs.models", fromlist=["Posicao"]).Posicao
        fields = (
            "id",
            "coordenada_x",
            "coordenada_y",
            "timestamp",
            "passo",
            "orientacao",
            "velocidade",
            "bateria",
        )
        read_only_fields = fields

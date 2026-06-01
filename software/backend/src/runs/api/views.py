"""ViewSets de runs. Views finas — toda lógica em use case/selector/service.

O streaming SSE NÃO está aqui: vive em `runs/api/sse.py` como view Django plain,
para escapar do `EnvelopeRenderer` (que serializa tudo em JSON envelope).
"""
from rest_framework import status
from rest_framework.decorators import action
from rest_framework.response import Response
from rest_framework.viewsets import ReadOnlyModelViewSet

from runs.api.serializers import TentativaSerializer
from runs.selectors import get_tentativa_by_id, list_tentativas
from runs.services.snapshot import build_snapshot
from runs.tasks import parar_corrida, simular_corrida
from runs.api.serializers import PosicaoSerializer
from runs.models import Micromouse, Labirinto, Posicao
from rest_framework.viewsets import ReadOnlyModelViewSet
from rest_framework import mixins
from rest_framework import serializers


class MicromouseViewSet(ReadOnlyModelViewSet):
    queryset = Micromouse.objects.all()
    serializer_class = __import__("runs.api.serializers", fromlist=["MicromouseSerializer"]).MicromouseSerializer


class LabirintoViewSet(ReadOnlyModelViewSet):
    queryset = Labirinto.objects.all()
    serializer_class = __import__("runs.api.serializers", fromlist=["LabirintoSerializer"]).LabirintoSerializer


class TentativaViewSet(ReadOnlyModelViewSet):
    queryset = list_tentativas()
    serializer_class = TentativaSerializer
    filterset_fields = ["status", "sucesso", "micromouse", "labirinto"]
    ordering_fields = ["created_at", "tempo_inicio"]

    @action(detail=True, methods=["get"])
    def snapshot(self, request, pk=None):
        """Snapshot inicial pro dashboard (maze + pose + métricas)."""
        tentativa = get_tentativa_by_id(tentativa_id=str(pk))
        return Response(build_snapshot(tentativa), status=status.HTTP_200_OK)

    @action(detail=True, methods=["post"])
    def comando(self, request, pk=None):
        """Controle remoto (RF22): iniciar/parar a corrida.

        Enquanto não há firmware, START dispara o simulador (task Celery) que faz
        o papel do robô; STOP sinaliza a parada. Quando o robô real existir, basta
        trocar por publicar o comando em `topic_comando` (já temos o client MQTT).
        """
        tentativa = get_tentativa_by_id(tentativa_id=str(pk))
        acao = request.data.get("acao")
        if acao == "start":
            simular_corrida.delay(str(tentativa.id))
        elif acao == "stop":
            parar_corrida(str(tentativa.id))
        return Response({"acao": acao}, status=status.HTTP_202_ACCEPTED)

    @action(detail=True, methods=["get"])
    def posicoes(self, request, pk=None):
        tentativa = get_tentativa_by_id(tentativa_id=str(pk))
        qs = Posicao.objects.filter(tentativa=tentativa).order_by("passo")
        serializer = PosicaoSerializer(qs, many=True)
        return Response(serializer.data)

    @action(detail=True, methods=["get"])
    def trajetoria(self, request, pk=None):
        tentativa = get_tentativa_by_id(tentativa_id=str(pk))
        qs = Posicao.objects.filter(tentativa=tentativa).order_by("passo")
        data = [
            {
                "x": p.coordenada_x,
                "y": p.coordenada_y,
                "timestamp": p.timestamp,
                "step": p.passo,
                "orientation": p.orientacao,
                "velocity": p.velocidade,
                "battery": p.bateria,
            }
            for p in qs
        ]
        return Response(data)

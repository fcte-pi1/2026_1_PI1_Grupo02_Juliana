"""ViewSets de runs. Views finas — toda lógica em use case/selector/service.

O streaming SSE NÃO está aqui: vive em `runs/api/sse.py` como view Django plain,
para escapar do `EnvelopeRenderer` (que serializa tudo em JSON envelope).
"""
import json

from django.conf import settings
from django.utils import timezone
from rest_framework import mixins, status
from rest_framework.decorators import action
from rest_framework.response import Response
from rest_framework.viewsets import ReadOnlyModelViewSet

import paho.mqtt.publish as mqtt_publish

from runs.api.serializers import PosicaoSerializer, TentativaSerializer
from runs.models import Labirinto, Micromouse, Posicao, Tentativa
from runs.selectors import get_tentativa_by_id, list_tentativas
from runs.services.snapshot import build_snapshot
from runs.tasks import parar_corrida, simular_corrida


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

    @action(detail=False, methods=["post"])
    def iniciar(self, request):
        """Cria uma nova Tentativa e inicia a corrida.

        Modo hardware (MICROMOUSE_MQTT_ID preenchido no .env):
          - Publica {"acao":"start","run_id":"<uuid>"} em micromouse/<id>/comando.
          - O firmware recebe, inicia a navegação autônoma e começa a enviar telemetria.

        Modo simulador (MICROMOUSE_MQTT_ID vazio):
          - Dispara a task Celery `simular_corrida` que faz o papel do robô.
        """
        mqtt_id = getattr(settings, "MICROMOUSE_MQTT_ID", "")

        if mqtt_id:
            mm, _ = Micromouse.objects.get_or_create(
                nome=f"Micromouse-{mqtt_id}", defaults={"algoritmo": "Flood Fill"}
            )
        else:
            mm, _ = Micromouse.objects.get_or_create(
                nome="Mouse-Sim", defaults={"algoritmo": "Flood Fill"}
            )

        dimensao = int(request.data.get("dimensao", 16))
        if dimensao not in (4, 8, 16):
            dimensao = 16
        lab, _ = Labirinto.objects.get_or_create(
            nome=f"Labirinto-{dimensao}x{dimensao}", defaults={"dimensao": dimensao}
        )
        tentativa = Tentativa.objects.create(micromouse=mm, labirinto=lab, tempo_inicio=timezone.now())

        if mqtt_id:
            topic = f"{settings.MQTT_BASE_TOPIC}/{mqtt_id}/comando"
            payload = json.dumps({"acao": "start", "run_id": str(tentativa.id)})
            auth = (
                {"username": settings.MQTT_USERNAME, "password": settings.MQTT_PASSWORD}
                if settings.MQTT_USERNAME
                else None
            )
            mqtt_publish.single(
                topic,
                payload=payload,
                qos=1,
                hostname=settings.MQTT_HOST,
                port=settings.MQTT_PORT,
                auth=auth,
            )
        else:
            simular_corrida.delay(str(tentativa.id))

        return Response(
            TentativaSerializer(tentativa).data, status=status.HTTP_201_CREATED
        )

    @action(detail=True, methods=["post"])
    def comando(self, request, pk=None):
        """Controle remoto (RF22): parar uma corrida em andamento.

        Hardware real (MICROMOUSE_MQTT_ID): publica {"acao":"stop"} via MQTT.
        Simulador: sinaliza a task Celery via flag Redis.
        """
        tentativa = get_tentativa_by_id(tentativa_id=str(pk))
        acao = request.data.get("acao")
        mqtt_id = getattr(settings, "MICROMOUSE_MQTT_ID", "")

        if acao == "stop":
            if mqtt_id:
                topic = f"{settings.MQTT_BASE_TOPIC}/{mqtt_id}/comando"
                auth = (
                    {"username": settings.MQTT_USERNAME, "password": settings.MQTT_PASSWORD}
                    if settings.MQTT_USERNAME
                    else None
                )
                mqtt_publish.single(
                    topic,
                    payload=json.dumps({"acao": "stop", "run_id": str(tentativa.id)}),
                    qos=1,
                    hostname=settings.MQTT_HOST,
                    port=settings.MQTT_PORT,
                    auth=auth,
                )
            else:
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

from django.urls import path
from rest_framework.routers import DefaultRouter

from runs.api.sse import telemetry_stream
from runs.api.views import TentativaViewSet

router = DefaultRouter()
router.register(r"tentativas", TentativaViewSet, basename="tentativa")
router.register(r"micromouses", __import__("runs.api.views", fromlist=["MicromouseViewSet"]).MicromouseViewSet, basename="micromouse")
router.register(r"labirintos", __import__("runs.api.views", fromlist=["LabirintoViewSet"]).LabirintoViewSet, basename="labirinto")

urlpatterns = [
    # SSE fora do router (view Django plain, escapa do EnvelopeRenderer).
    path("tentativas/<str:pk>/stream/", telemetry_stream, name="tentativa-stream"),
    *router.urls,
]

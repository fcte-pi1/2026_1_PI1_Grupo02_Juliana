from django.urls import path
from rest_framework.routers import DefaultRouter

from runs.api.sse import telemetry_stream
from runs.api.views import TentativaViewSet

router = DefaultRouter()
router.register(r"tentativas", TentativaViewSet, basename="tentativa")

urlpatterns = [
    # SSE fora do router (view Django plain, escapa do EnvelopeRenderer).
    path("tentativas/<str:pk>/stream/", telemetry_stream, name="tentativa-stream"),
    *router.urls,
]

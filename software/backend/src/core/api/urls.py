"""URLs internas do core. Hoje, vazio — `config/urls.py` aponta direto pra
`core.api.views.health_view`. Mantido pra extensão futura (caso o core ganhe
mais endpoints transversais).
"""
from django.urls import path

from .views import health_view

urlpatterns = [
    path("health/", health_view, name="core-health"),
]

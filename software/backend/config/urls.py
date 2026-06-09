"""Top-level URL routing.

Apps de domínio expõem suas próprias rotas via `include(...)`. Este arquivo
só monta:
- /admin/
- /api/v1/health/         (core healthcheck simples — AllowAny)
- /api/v1/token/          (JWT obtain)
- /api/v1/token/refresh/  (JWT refresh)
- /api/v1/healthcheck/    (app `healthcheck`, se existir)
- /api/v1/runs/           (app `runs` — telemetria do micromouse)
"""
import contextlib

from django.contrib import admin
from django.urls import include, path
from rest_framework_simplejwt.views import TokenObtainPairView, TokenRefreshView

from core.observability.health import health_view

urlpatterns = [
    path("admin/", admin.site.urls),
    path("api/v1/health/", health_view, name="health"),
    path("api/v1/token/", TokenObtainPairView.as_view(), name="token_obtain_pair"),
    path("api/v1/token/refresh/", TokenRefreshView.as_view(), name="token_refresh"),
]

# `healthcheck` app pode ainda não existir durante bootstrap. Inclui se disponível.
with contextlib.suppress(ImportError):
    urlpatterns.append(path("api/v1/healthcheck/", include("healthcheck.api.urls")))

with contextlib.suppress(ImportError):
    urlpatterns.append(path("api/v1/runs/", include("runs.api.urls")))

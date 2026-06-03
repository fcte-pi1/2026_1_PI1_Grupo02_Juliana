from django.apps import AppConfig


class HealthcheckConfig(AppConfig):
    name = "healthcheck"
    verbose_name = "Healthcheck"
    default_auto_field = "django.db.models.BigAutoField"

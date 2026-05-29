"""Celery application instance — autodiscover tasks across installed apps.

Tasks themselves live under each app (`<app>/tasks.py`). This module is
only responsible for the app instance and configuration glue.
"""
import os

from celery import Celery

os.environ.setdefault("DJANGO_SETTINGS_MODULE", "config.settings.local")

app = Celery("config")
app.config_from_object("django.conf:settings", namespace="CELERY")
app.autodiscover_tasks()

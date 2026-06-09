"""WSGI config for the Micromouse backend project."""
import os
import sys
from pathlib import Path

from django.core.wsgi import get_wsgi_application

BASE_DIR = Path(__file__).resolve().parent.parent
src_path = str(BASE_DIR / "src")
if src_path not in sys.path:
    sys.path.insert(0, src_path)

os.environ.setdefault("DJANGO_SETTINGS_MODULE", "config.settings.local")

application = get_wsgi_application()

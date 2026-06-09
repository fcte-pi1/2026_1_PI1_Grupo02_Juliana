"""Test settings — Celery eager, MD5 hasher, suffix _test no DB."""
from .base import *  # noqa: F401, F403
from .base import DATABASES

DEBUG = False

# Celery roda inline durante testes
CELERY_TASK_ALWAYS_EAGER = True
CELERY_TASK_EAGER_PROPAGATES = True

# DB de teste — sufixo _test pra não colidir
DATABASES = {
    **DATABASES,
    "default": {
        **DATABASES["default"],
        "NAME": f"{DATABASES['default']['NAME']}_test",
        "TEST": {"NAME": f"{DATABASES['default']['NAME']}_test"},
    },
}

# Hash mais rápido só pra teste
PASSWORD_HASHERS = [
    "django.contrib.auth.hashers.MD5PasswordHasher",
]

"""Local development settings."""
from .base import *  # noqa: F401, F403
from .base import INSTALLED_APPS, MIDDLEWARE, REST_FRAMEWORK

DEBUG = True
ALLOWED_HOSTS = ["*"]

INSTALLED_APPS = INSTALLED_APPS + [
    "debug_toolbar",
    "django_extensions",
]

MIDDLEWARE = MIDDLEWARE + [
    "debug_toolbar.middleware.DebugToolbarMiddleware",
]

# Em DEBUG, exibir BrowsableAPIRenderer também.
REST_FRAMEWORK = {
    **REST_FRAMEWORK,
    "DEFAULT_RENDERER_CLASSES": (
        "core.api.renderers.EnvelopeRenderer",
        "rest_framework.renderers.BrowsableAPIRenderer",
    ),
}

INTERNAL_IPS = ["127.0.0.1"]

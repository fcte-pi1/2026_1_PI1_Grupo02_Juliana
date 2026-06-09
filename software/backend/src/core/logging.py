"""Configuração de logging — loguru como sink único.

Rationale: o stdlib ``logging`` é usado por Django, DRF, Celery, gunicorn e
psycopg. Em vez de manter duas pilhas, redirecionamos tudo para o ``loguru``
via :class:`InterceptHandler`. Assim temos uma única configuração de sinks
(pretty em dev, JSON em prod) com contexto (``request_id``) já carregado
pelos middlewares.

Uso: ``configure_logging()`` é chamado no fim de ``settings/base.py``.
O modo é decidido pelo settings (DEBUG → ``pretty``; senão → ``json``)
e o nível pelo env ``LOG_LEVEL`` (default ``INFO``).
"""
import logging
import sys
from typing import Literal

from loguru import logger

LogMode = Literal["pretty", "json"]


class InterceptHandler(logging.Handler):
    """Redireciona registros do ``logging`` stdlib para ``loguru``."""

    def emit(self, record: logging.LogRecord) -> None:  # pragma: no cover - glue
        try:
            level: str | int = logger.level(record.levelname).name
        except ValueError:
            level = record.levelno

        # Profundidade pra loguru reportar o caller real, não o handler.
        frame, depth = logging.currentframe(), 2
        while frame and frame.f_code.co_filename == logging.__file__:
            frame = frame.f_back
            depth += 1

        logger.opt(depth=depth, exception=record.exc_info).log(level, record.getMessage())


def configure_logging(mode: LogMode = "pretty", level: str = "INFO") -> None:
    """Configura o loguru e instala o ``InterceptHandler``.

    Args:
        mode: ``"pretty"`` (dev, colorido, multiline) ou ``"json"`` (prod,
            um objeto JSON por linha — ingestível em Loki/Grafana/Datadog).
        level: nível mínimo das mensagens processadas.
    """
    logger.remove()

    if mode == "json":
        logger.add(
            sys.stderr,
            level=level,
            serialize=True,
            backtrace=False,
            diagnose=False,
        )
    else:
        logger.add(
            sys.stderr,
            level=level,
            colorize=True,
            backtrace=True,
            diagnose=True,
            format=(
                "<green>{time:YYYY-MM-DD HH:mm:ss.SSS}</green> | "
                "<level>{level: <8}</level> | "
                "<cyan>{extra[request_id]}</cyan> | "
                "<cyan>{name}</cyan>:<cyan>{function}</cyan>:<cyan>{line}</cyan> - "
                "<level>{message}</level>"
            ),
        )

    # Contexto default — evita ``KeyError`` em mensagens fora do request
    # cycle (startup, management commands, signals).
    logger.configure(extra={"request_id": "-"})

    # Plugga o logging stdlib inteiro no loguru.
    logging.basicConfig(handlers=[InterceptHandler()], level=0, force=True)


__all__ = ["InterceptHandler", "configure_logging"]

"""Factory de Fetcher.

Hoje sempre devolve HttpxFetcher. Quando houver PlaywrightFetcher (sites JS),
esta função decide com base em config/parâmetro qual implementação usar.
"""

from .base import Fetcher
from .httpx_fetcher import HttpxFetcher


def get_fetcher(*, timeout: float = 10.0) -> Fetcher:
    """Retorna a implementação default de Fetcher (httpx)."""
    return HttpxFetcher(timeout=timeout)

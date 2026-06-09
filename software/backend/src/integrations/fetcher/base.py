"""Interface abstrata da categoria fetcher.

Define o contrato que toda implementação concreta (HttpxFetcher, futuramente
PlaywrightFetcher) deve cumprir. Use cases dependem desta interface, não da
implementação.
"""

from dataclasses import dataclass


class FetcherError(Exception):
    """Erro genérico de fetch externo. Implementações devem re-raise como esta."""


@dataclass
class FetchResult:
    status_code: int
    body: str
    headers: dict


class Fetcher:
    """Interface abstrata para clientes HTTP da camada integrations."""

    def get(self, url: str) -> FetchResult:
        raise NotImplementedError

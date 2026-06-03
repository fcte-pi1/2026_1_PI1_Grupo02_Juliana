"""Implementação default do Fetcher usando httpx (sync).

httpx é a única lib HTTP permitida no backend Micromouse. Para sites com JS
pesado (SPA), futuramente teremos um PlaywrightFetcher na mesma interface.
"""

import httpx

from .base import Fetcher, FetcherError, FetchResult


class HttpxFetcher(Fetcher):
    def __init__(self, *, timeout: float = 10.0, follow_redirects: bool = True):
        self._timeout = timeout
        self._follow_redirects = follow_redirects

    def get(self, url: str) -> FetchResult:
        try:
            with httpx.Client(
                timeout=self._timeout,
                follow_redirects=self._follow_redirects,
            ) as client:
                response = client.get(url)
        except httpx.HTTPError as exc:
            raise FetcherError(f"GET {url} failed: {exc}") from exc

        return FetchResult(
            status_code=response.status_code,
            body=response.text,
            headers=dict(response.headers),
        )

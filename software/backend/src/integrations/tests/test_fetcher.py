"""Tests para integrations.fetcher — interface, factory e HttpxFetcher."""
from unittest.mock import MagicMock, patch

import httpx
import pytest

from integrations.fetcher.base import Fetcher, FetcherError, FetchResult
from integrations.fetcher.factory import get_fetcher
from integrations.fetcher.httpx_fetcher import HttpxFetcher


# ─── FetchResult ─────────────────────────────────────────────────────────────


def test_fetch_result_is_dataclass():
    result = FetchResult(status_code=200, body="ok", headers={"Content-Type": "text/plain"})
    assert result.status_code == 200
    assert result.body == "ok"
    assert result.headers["Content-Type"] == "text/plain"


# ─── FetcherError ─────────────────────────────────────────────────────────────


def test_fetcher_error_is_exception():
    err = FetcherError("network timeout")
    assert isinstance(err, Exception)
    assert str(err) == "network timeout"


# ─── Fetcher (interface) ──────────────────────────────────────────────────────


def test_fetcher_base_raises_not_implemented():
    fetcher = Fetcher()
    with pytest.raises(NotImplementedError):
        fetcher.get("http://example.com")


# ─── get_fetcher factory ──────────────────────────────────────────────────────


def test_get_fetcher_returns_httpx_fetcher():
    fetcher = get_fetcher()
    assert isinstance(fetcher, HttpxFetcher)


def test_get_fetcher_passes_timeout():
    fetcher = get_fetcher(timeout=5.0)
    assert fetcher._timeout == 5.0


# ─── HttpxFetcher ─────────────────────────────────────────────────────────────


def test_httpx_fetcher_get_success():
    mock_response = MagicMock()
    mock_response.status_code = 200
    mock_response.text = "hello"
    mock_response.headers = {"Content-Type": "text/html"}

    mock_client_instance = MagicMock()
    mock_client_instance.get.return_value = mock_response
    mock_client_instance.__enter__ = lambda s: s
    mock_client_instance.__exit__ = MagicMock(return_value=False)

    with patch("integrations.fetcher.httpx_fetcher.httpx.Client", return_value=mock_client_instance):
        result = HttpxFetcher().get("http://example.com")

    assert result.status_code == 200
    assert result.body == "hello"
    assert result.headers["Content-Type"] == "text/html"


def test_httpx_fetcher_get_raises_fetcher_error_on_http_error():
    mock_client_instance = MagicMock()
    mock_client_instance.get.side_effect = httpx.ConnectError("refused")
    mock_client_instance.__enter__ = lambda s: s
    mock_client_instance.__exit__ = MagicMock(return_value=False)

    with patch("integrations.fetcher.httpx_fetcher.httpx.Client", return_value=mock_client_instance):
        with pytest.raises(FetcherError) as exc_info:
            HttpxFetcher().get("http://bad-host")

    assert "GET http://bad-host failed" in str(exc_info.value)


def test_httpx_fetcher_default_timeout():
    fetcher = HttpxFetcher()
    assert fetcher._timeout == 10.0
    assert fetcher._follow_redirects is True


def test_httpx_fetcher_custom_timeout():
    fetcher = HttpxFetcher(timeout=3.0, follow_redirects=False)
    assert fetcher._timeout == 3.0
    assert fetcher._follow_redirects is False

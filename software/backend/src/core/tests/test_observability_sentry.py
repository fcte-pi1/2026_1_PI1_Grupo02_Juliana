"""Testes da integração Sentry.

Cobre:

* ``init_sentry`` retorna ``False`` com DSN vazio (no-op total).
* ``init_sentry`` retorna ``True`` e chama ``sentry_sdk.init`` com DSN populado.
* ``_before_send`` redige headers sensíveis (defesa em profundidade).
"""
from unittest.mock import patch

import pytest

from core.observability.sentry import _before_send, init_sentry


class TestInitSentry:
    def test_init_sentry_skips_when_dsn_empty(self):
        """DSN vazio não inicializa nada — dev humano roda sem."""
        with patch("core.observability.sentry.sentry_sdk.init") as mock_init:
            result = init_sentry(dsn="", environment="test")
        assert result is False
        mock_init.assert_not_called()

    def test_init_sentry_initializes_when_dsn_present(self):
        """DSN preenchido dispara ``sentry_sdk.init`` com kwargs corretos.

        Patcha ``sentry_sdk.init`` pra evitar subir client real (que
        ficaria tentando flush ao fim dos testes).
        """
        fake_dsn = "https://examplePublicKey@o0.ingest.sentry.io/0"
        with patch("core.observability.sentry.sentry_sdk.init") as mock_init:
            result = init_sentry(
                dsn=fake_dsn,
                environment="staging",
                release="abc123",
                traces_sample_rate=0.5,
            )
        assert result is True
        mock_init.assert_called_once()
        kwargs = mock_init.call_args.kwargs
        assert kwargs["dsn"] == fake_dsn
        assert kwargs["environment"] == "staging"
        assert kwargs["release"] == "abc123"
        assert kwargs["traces_sample_rate"] == 0.5
        assert kwargs["send_default_pii"] is False
        assert kwargs["before_send"] is not None
        assert len(kwargs["integrations"]) == 3  # Django + Celery + Logging


class TestBeforeSend:
    @pytest.mark.parametrize(
        "header_key",
        ["Authorization", "X-API-Key", "authorization", "x-api-key"],
    )
    def test_redacts_sensitive_headers(self, header_key):
        event = {
            "request": {
                "headers": {
                    header_key: "Bearer eyJhbGciOiSuperSecretTokenXYZ",
                    "Content-Type": "application/json",
                },
            },
        }
        result = _before_send(event, {})
        assert result["request"]["headers"][header_key] == "[REDACTED]"
        # Headers não sensíveis preservados.
        assert result["request"]["headers"]["Content-Type"] == "application/json"

    def test_no_request_block_is_fine(self):
        """Eventos sem ``request`` (ex.: errors de startup) passam sem explodir."""
        event: dict = {}
        result = _before_send(event, {})
        assert result is event
        assert "request" not in result

    def test_request_without_headers_is_fine(self):
        event: dict = {"request": {}}
        result = _before_send(event, {})
        assert result["request"] == {}

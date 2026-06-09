"""Tests para integrations.mqtt.client — funções de tópico e build_client."""
from unittest.mock import MagicMock, patch

import pytest


# ─── Funções de tópico ───────────────────────────────────────────────────────


@pytest.fixture(autouse=True)
def set_mqtt_settings(settings):
    settings.MQTT_BASE_TOPIC = "micromouse"
    settings.MQTT_HOST = "localhost"
    settings.MQTT_PORT = 1883
    settings.MQTT_USERNAME = ""
    settings.MQTT_PASSWORD = ""


def test_topic_telemetria_format():
    from integrations.mqtt.client import topic_telemetria
    assert topic_telemetria("mouse-1") == "micromouse/mouse-1/telemetria"


def test_topic_evento_format():
    from integrations.mqtt.client import topic_evento
    assert topic_evento("mouse-1") == "micromouse/mouse-1/evento"


def test_topic_status_format():
    from integrations.mqtt.client import topic_status
    assert topic_status("mouse-1") == "micromouse/mouse-1/status"


def test_topic_comando_format():
    from integrations.mqtt.client import topic_comando
    assert topic_comando("mouse-1") == "micromouse/mouse-1/comando"


def test_topic_uses_configured_base_topic(settings):
    settings.MQTT_BASE_TOPIC = "custom/base"
    from integrations.mqtt.client import topic_telemetria
    assert topic_telemetria("abc") == "custom/base/abc/telemetria"


# ─── build_client ─────────────────────────────────────────────────────────────


def test_build_client_connects_without_auth(settings):
    settings.MQTT_USERNAME = ""

    mock_client = MagicMock()

    with patch("integrations.mqtt.client.mqtt.Client", return_value=mock_client):
        from integrations.mqtt.client import build_client
        result = build_client(client_id="test-client")

    mock_client.connect.assert_called_once_with("localhost", 1883, keepalive=60)
    mock_client.username_pw_set.assert_not_called()
    assert result is mock_client


def test_build_client_sets_credentials_when_username_configured(settings):
    settings.MQTT_USERNAME = "user"
    settings.MQTT_PASSWORD = "secret"

    mock_client = MagicMock()

    with patch("integrations.mqtt.client.mqtt.Client", return_value=mock_client):
        from integrations.mqtt.client import build_client
        build_client(client_id="test-client")

    mock_client.username_pw_set.assert_called_once_with("user", "secret")
    mock_client.connect.assert_called_once()

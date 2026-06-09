"""Tests para runs.realtime — ponte Redis pub/sub para SSE.

Redis é mockado para não exigir infra externa.
"""
import json
from unittest.mock import MagicMock, patch

from runs.realtime import channel, publish_snapshot, subscribe


# ─── channel ─────────────────────────────────────────────────────────────────


def test_channel_format():
    assert channel("abc-123") == "telemetry:abc-123"


def test_channel_different_ids():
    assert channel("id-1") != channel("id-2")


# ─── publish_snapshot ────────────────────────────────────────────────────────


def test_publish_snapshot_calls_redis_publish():
    mock_redis = MagicMock()
    snapshot = {"tentativa_id": "abc", "explored": 2}

    with patch("runs.realtime._client", return_value=mock_redis):
        publish_snapshot("abc", snapshot)

    mock_redis.publish.assert_called_once()
    call_args = mock_redis.publish.call_args
    assert call_args[0][0] == "telemetry:abc"
    # Payload deve ser JSON serializável
    published = json.loads(call_args[0][1])
    assert published["tentativa_id"] == "abc"
    assert published["explored"] == 2


def test_publish_snapshot_serializes_non_json_values():
    from datetime import datetime, UTC

    mock_redis = MagicMock()
    snapshot = {"ts": datetime.now(UTC)}

    with patch("runs.realtime._client", return_value=mock_redis):
        publish_snapshot("run-1", snapshot)

    # Deve não levantar erro — datetime serializado via default=str
    call_args = mock_redis.publish.call_args
    published_str = call_args[0][1]
    assert isinstance(published_str, str)


# ─── subscribe ───────────────────────────────────────────────────────────────


def test_subscribe_returns_pubsub_subscribed_to_channel():
    mock_pubsub = MagicMock()
    mock_redis = MagicMock()
    mock_redis.pubsub.return_value = mock_pubsub

    with patch("runs.realtime._client", return_value=mock_redis):
        result = subscribe("run-42")

    mock_pubsub.subscribe.assert_called_once_with("telemetry:run-42")
    assert result is mock_pubsub

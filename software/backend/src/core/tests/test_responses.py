"""Tests para core.api.responses — build_envelope e error_envelope."""
from types import SimpleNamespace

from core.api.responses import build_envelope, error_envelope


# ─── build_envelope ──────────────────────────────────────────────────────────


def test_build_envelope_success_is_true():
    envelope = build_envelope(data={"key": "value"})
    assert envelope["success"] is True


def test_build_envelope_data_is_forwarded():
    data = {"foo": "bar", "count": 42}
    envelope = build_envelope(data=data)
    assert envelope["data"] == data


def test_build_envelope_errors_is_none():
    envelope = build_envelope(data=None)
    assert envelope["errors"] is None


def test_build_envelope_meta_has_timestamp():
    envelope = build_envelope(data=None)
    assert "timestamp" in envelope["meta"]
    assert envelope["meta"]["timestamp"]


def test_build_envelope_meta_request_id_none_without_request():
    envelope = build_envelope(data=None)
    assert envelope["meta"]["request_id"] is None


def test_build_envelope_meta_request_id_from_request():
    fake_request = SimpleNamespace(request_id="test-id-123")
    envelope = build_envelope(data=None, request=fake_request)
    assert envelope["meta"]["request_id"] == "test-id-123"


def test_build_envelope_pagination_none_by_default():
    envelope = build_envelope(data=[])
    assert envelope["pagination"] is None


def test_build_envelope_pagination_forwarded():
    pagination = {"count": 10, "page": 1}
    envelope = build_envelope(data=[], pagination=pagination)
    assert envelope["pagination"] == pagination


def test_build_envelope_warnings_none_by_default():
    envelope = build_envelope(data=None)
    assert envelope["warnings"] is None


def test_build_envelope_warnings_forwarded():
    envelope = build_envelope(data=None, warnings=["deprecated field"])
    assert envelope["warnings"] == ["deprecated field"]


# ─── error_envelope ──────────────────────────────────────────────────────────


def test_error_envelope_success_is_false():
    envelope = error_envelope("SomeError", "something went wrong")
    assert envelope["success"] is False


def test_error_envelope_data_is_none():
    envelope = error_envelope("SomeError", "msg")
    assert envelope["data"] is None


def test_error_envelope_errors_populated():
    envelope = error_envelope("NotFoundError", "resource missing")
    assert len(envelope["errors"]) == 1
    err = envelope["errors"][0]
    assert err["code"] == "NotFoundError"
    assert err["message"] == "resource missing"
    assert err["fields"] == {}


def test_error_envelope_fields_forwarded():
    envelope = error_envelope("ValidationError", "invalid", fields={"email": ["required"]})
    assert envelope["errors"][0]["fields"] == {"email": ["required"]}


def test_error_envelope_fields_default_empty_dict():
    envelope = error_envelope("SomeError", "msg")
    assert envelope["errors"][0]["fields"] == {}


def test_error_envelope_meta_has_timestamp():
    envelope = error_envelope("E", "m")
    assert "timestamp" in envelope["meta"]


def test_error_envelope_request_id_from_request():
    fake_request = SimpleNamespace(request_id="req-abc")
    envelope = error_envelope("E", "m", request=fake_request)
    assert envelope["meta"]["request_id"] == "req-abc"

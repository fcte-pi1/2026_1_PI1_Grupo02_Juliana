"""Smoke tests do `custom_exception_handler` — mapeamento erro → status."""
from core.errors import (
    ApplicationError,
    NotFoundError,
    PermissionDeniedError,
    custom_exception_handler,
)


def test_application_error_maps_to_400():
    response = custom_exception_handler(ApplicationError("test"), context={})
    assert response is not None
    assert response.status_code == 400
    assert response.data["code"] == "ApplicationError"
    assert response.data["message"] == "test"
    assert response.data["fields"] == {}


def test_not_found_error_maps_to_404():
    response = custom_exception_handler(NotFoundError("missing"), context={})
    assert response is not None
    assert response.status_code == 404
    assert response.data["code"] == "NotFoundError"
    assert response.data["message"] == "missing"


def test_permission_denied_error_maps_to_403():
    response = custom_exception_handler(PermissionDeniedError("nope"), context={})
    assert response is not None
    assert response.status_code == 403
    assert response.data["code"] == "PermissionDeniedError"
    assert response.data["message"] == "nope"

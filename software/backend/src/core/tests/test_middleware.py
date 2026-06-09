"""Tests para core.middleware.RequestIDMiddleware."""
import uuid

from django.test import RequestFactory

from core.middleware import RequestIDMiddleware


def _make_middleware(response_factory=None):
    """Retorna uma instância do middleware com get_response simples."""
    if response_factory is None:
        from django.http import HttpResponse

        def get_response(request):
            return HttpResponse("ok")
    else:
        get_response = response_factory

    return RequestIDMiddleware(get_response)


def test_request_id_generated_when_absent():
    rf = RequestFactory()
    request = rf.get("/")

    middleware = _make_middleware()
    response = middleware(request)

    assert hasattr(request, "request_id")
    assert request.request_id
    # Deve ser um UUID válido
    uuid.UUID(request.request_id)


def test_request_id_propagated_from_incoming_header():
    rf = RequestFactory()
    existing_id = str(uuid.uuid4())
    request = rf.get("/", HTTP_X_REQUEST_ID=existing_id)

    middleware = _make_middleware()
    middleware(request)

    assert request.request_id == existing_id


def test_response_has_x_request_id_header():
    rf = RequestFactory()
    request = rf.get("/")

    middleware = _make_middleware()
    response = middleware(request)

    assert "X-Request-ID" in response
    assert response["X-Request-ID"] == request.request_id


def test_response_x_request_id_matches_incoming_header():
    rf = RequestFactory()
    existing_id = "fixed-request-id-value"
    request = rf.get("/", HTTP_X_REQUEST_ID=existing_id)

    middleware = _make_middleware()
    response = middleware(request)

    assert response["X-Request-ID"] == existing_id


def test_different_requests_get_different_ids():
    rf = RequestFactory()
    request1 = rf.get("/")
    request2 = rf.get("/")

    middleware = _make_middleware()
    middleware(request1)
    middleware(request2)

    assert request1.request_id != request2.request_id

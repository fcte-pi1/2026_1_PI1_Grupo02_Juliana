"""Smoke tests do `RunCheck` — happy path + falhas + erros de domínio."""
import uuid

import pytest

from core.errors import ApplicationError, NotFoundError
from healthcheck.tests.factories import ServiceCheckFactory
from healthcheck.use_cases.run_check import RunCheck
from integrations.fetcher.base import FetcherError, FetchResult


class FakeFetcher:
    """Fake mínimo do Fetcher — controla status_code ou levanta FetcherError."""

    def __init__(self, status_code: int = 200, raises: Exception | None = None):
        self.status_code = status_code
        self.raises = raises

    def get(self, url: str) -> FetchResult:
        if self.raises is not None:
            raise self.raises
        return FetchResult(status_code=self.status_code, body="", headers={})


@pytest.mark.django_db
def test_run_check_marks_ok_when_status_matches():
    check = ServiceCheckFactory(expected_status=200)
    RunCheck(fetcher=FakeFetcher(status_code=200)).execute(check_id=str(check.id))
    check.refresh_from_db()
    assert check.last_status == "ok"
    assert check.last_status_code == 200
    assert check.last_error == ""
    assert check.last_checked_at is not None


@pytest.mark.django_db
def test_run_check_marks_fail_when_status_differs():
    check = ServiceCheckFactory(expected_status=200)
    RunCheck(fetcher=FakeFetcher(status_code=500)).execute(check_id=str(check.id))
    check.refresh_from_db()
    assert check.last_status == "fail"
    assert check.last_status_code == 500


@pytest.mark.django_db
def test_run_check_marks_fail_when_fetcher_errors():
    check = ServiceCheckFactory(expected_status=200)
    fetcher = FakeFetcher(raises=FetcherError("connection refused"))
    RunCheck(fetcher=fetcher).execute(check_id=str(check.id))
    check.refresh_from_db()
    assert check.last_status == "fail"
    assert check.last_status_code is None
    assert "connection refused" in check.last_error


@pytest.mark.django_db
def test_run_check_raises_when_check_inactive():
    check = ServiceCheckFactory(is_active=False)
    with pytest.raises(ApplicationError):
        RunCheck(fetcher=FakeFetcher(200)).execute(check_id=str(check.id))


@pytest.mark.django_db
def test_run_check_raises_not_found_when_id_invalid():
    with pytest.raises(NotFoundError):
        RunCheck(fetcher=FakeFetcher(200)).execute(check_id=str(uuid.uuid4()))

"""Use case `RunCheck` — dispara um GET no endpoint cadastrado e atualiza estado.

Depende da interface `Fetcher` (não da implementação). Em testes, injete um
fake via `RunCheck(fetcher=...)`. Em produção, `factory.get_fetcher()` decide.
"""
from django.utils import timezone

from core.errors import ApplicationError
from healthcheck.models import ServiceCheck
from healthcheck.selectors import get_check_by_id
from integrations.fetcher.base import Fetcher, FetcherError
from integrations.fetcher.factory import get_fetcher


class RunCheck:
    def __init__(self, *, fetcher: Fetcher | None = None):
        self._fetcher = fetcher or get_fetcher()

    def execute(self, *, check_id: str) -> ServiceCheck:
        check = get_check_by_id(check_id=check_id)
        if not check.is_active:
            raise ApplicationError(f"Check '{check.name}' is not active.")

        try:
            result = self._fetcher.get(check.url)
            check.last_status_code = result.status_code
            check.last_status = (
                ServiceCheck.Status.OK
                if result.status_code == check.expected_status
                else ServiceCheck.Status.FAIL
            )
            check.last_error = ""
        except FetcherError as exc:
            check.last_status = ServiceCheck.Status.FAIL
            check.last_status_code = None
            check.last_error = str(exc)

        check.last_checked_at = timezone.now()
        check.save(
            update_fields=[
                "last_status",
                "last_status_code",
                "last_error",
                "last_checked_at",
                "updated_at",
            ]
        )
        return check

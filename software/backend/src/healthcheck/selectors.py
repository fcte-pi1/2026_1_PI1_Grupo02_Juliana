"""Queries puras sobre `ServiceCheck`. Sem regra de negócio — só leitura."""
from django.db.models import QuerySet

from core.errors import NotFoundError
from healthcheck.models import ServiceCheck


def get_check_by_id(*, check_id: str) -> ServiceCheck:
    try:
        return ServiceCheck.objects.get(id=check_id)
    except ServiceCheck.DoesNotExist as exc:
        raise NotFoundError(f"ServiceCheck {check_id} not found.") from exc


def list_active_checks() -> QuerySet[ServiceCheck]:
    return ServiceCheck.objects.filter(is_active=True)

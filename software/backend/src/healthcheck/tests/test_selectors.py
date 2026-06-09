"""Smoke tests dos selectors — lookups + filtros."""
import uuid

import pytest

from core.errors import NotFoundError
from healthcheck.selectors import get_check_by_id, list_active_checks
from healthcheck.tests.factories import ServiceCheckFactory


@pytest.mark.django_db
def test_get_check_by_id_returns_check():
    check = ServiceCheckFactory()
    found = get_check_by_id(check_id=str(check.id))
    assert found.id == check.id


@pytest.mark.django_db
def test_get_check_by_id_raises_not_found():
    with pytest.raises(NotFoundError):
        get_check_by_id(check_id=str(uuid.uuid4()))


@pytest.mark.django_db
def test_list_active_checks_filters_inactive():
    ServiceCheckFactory.create_batch(2, is_active=True)
    ServiceCheckFactory(is_active=False)
    actives = list(list_active_checks())
    assert len(actives) == 2
    assert all(c.is_active for c in actives)

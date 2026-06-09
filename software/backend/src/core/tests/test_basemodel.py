"""Smoke tests do BaseModel — UUID v7 (ou v4 fallback) + timestamps."""
import uuid

import pytest

from healthcheck.tests.factories import ServiceCheckFactory


@pytest.mark.django_db
def test_basemodel_assigns_uuid_v7_id():
    check = ServiceCheckFactory()
    assert isinstance(check.id, uuid.UUID)
    # Tolera v7 (Python 3.14+) ou v4 (fallback antigo).
    assert check.id.version in (4, 7)


@pytest.mark.django_db
def test_basemodel_sets_timestamps():
    check = ServiceCheckFactory()
    assert check.created_at is not None
    assert check.updated_at is not None

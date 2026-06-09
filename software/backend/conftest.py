"""Root conftest — shared fixtures for the whole backend test suite."""
import sys
from pathlib import Path

import pytest

BASE_DIR = Path(__file__).resolve().parent
SRC_DIR = BASE_DIR / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))


@pytest.fixture
def api_client():
    """Unauthenticated DRF APIClient."""
    from rest_framework.test import APIClient

    return APIClient()


@pytest.fixture
def auth_client(db):
    """Authenticated DRF APIClient — common user, force_authenticate."""
    from django.contrib.auth import get_user_model
    from rest_framework.test import APIClient

    User = get_user_model()
    user = User.objects.create_user(
        username="testuser",
        email="testuser@example.com",
        password="testpass123",
    )
    client = APIClient()
    client.force_authenticate(user=user)
    client.user = user
    return client


@pytest.fixture
def superuser_client(db):
    """Authenticated DRF APIClient — staff/superuser, force_authenticate."""
    from django.contrib.auth import get_user_model
    from rest_framework.test import APIClient

    User = get_user_model()
    user = User.objects.create_superuser(
        username="adminuser",
        email="adminuser@example.com",
        password="adminpass123",
    )
    client = APIClient()
    client.force_authenticate(user=user)
    client.user = user
    return client

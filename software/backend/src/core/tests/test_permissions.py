"""Tests para `core.permissions` — roles via Group + ownership + read-only."""
from types import SimpleNamespace

import factory
import pytest
from django.contrib.auth.models import AnonymousUser, Group
from rest_framework.test import APIRequestFactory

from core.permissions import HasAnyRole, HasRole, IsObjectOwner, ReadOnly
from core.tests.factories import UserFactory


class GroupFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Group
        django_get_or_create = ("name",)

    name = "admin"


@pytest.fixture
def rf():
    return APIRequestFactory()


@pytest.fixture
def view():
    """Mock minimal de view — permission classes não usam atributos reais."""
    return SimpleNamespace()


def _make_request(rf, *, method="get", user=None):
    factory_method = getattr(rf, method)
    request = factory_method("/")
    request.user = user if user is not None else AnonymousUser()
    return request


# ---------- HasRole ----------


@pytest.mark.django_db
def test_has_role_grants_when_user_in_group(rf, view):
    user = UserFactory()
    group = GroupFactory(name="admin")
    user.groups.add(group)

    perm = HasRole("admin")()
    request = _make_request(rf, user=user)

    assert perm.has_permission(request, view) is True


@pytest.mark.django_db
def test_has_role_denies_when_user_not_in_group(rf, view):
    user = UserFactory()
    GroupFactory(name="admin")  # grupo existe, user não está nele

    perm = HasRole("admin")()
    request = _make_request(rf, user=user)

    assert perm.has_permission(request, view) is False


@pytest.mark.django_db
def test_has_role_denies_when_user_anonymous(rf, view):
    GroupFactory(name="admin")

    perm = HasRole("admin")()
    request = _make_request(rf, user=AnonymousUser())

    assert perm.has_permission(request, view) is False


# ---------- HasAnyRole ----------


@pytest.mark.django_db
def test_has_any_role_grants_when_user_has_one_of_them(rf, view):
    user = UserFactory()
    gestor = GroupFactory(name="gestor")
    GroupFactory(name="admin")
    user.groups.add(gestor)

    perm = HasAnyRole("admin", "gestor")()
    request = _make_request(rf, user=user)

    assert perm.has_permission(request, view) is True


@pytest.mark.django_db
def test_has_any_role_denies_when_user_has_none(rf, view):
    user = UserFactory()
    GroupFactory(name="admin")
    GroupFactory(name="gestor")
    user.groups.add(GroupFactory(name="membro"))

    perm = HasAnyRole("admin", "gestor")()
    request = _make_request(rf, user=user)

    assert perm.has_permission(request, view) is False


@pytest.mark.django_db
def test_has_any_role_denies_when_user_anonymous(rf, view):
    perm = HasAnyRole("admin", "gestor")()
    request = _make_request(rf, user=AnonymousUser())

    assert perm.has_permission(request, view) is False


# ---------- IsObjectOwner ----------


@pytest.mark.django_db
def test_is_object_owner_grants_when_owner_field_matches(rf, view):
    user = UserFactory()
    obj = SimpleNamespace(owner=user)

    perm = IsObjectOwner()
    request = _make_request(rf, user=user)

    assert perm.has_object_permission(request, view, obj) is True


@pytest.mark.django_db
def test_is_object_owner_falls_back_to_created_by(rf, view):
    user = UserFactory()
    obj = SimpleNamespace(created_by=user)  # sem `owner`

    perm = IsObjectOwner()
    request = _make_request(rf, user=user)

    assert perm.has_object_permission(request, view, obj) is True


@pytest.mark.django_db
def test_is_object_owner_falls_back_to_user_attr(rf, view):
    user = UserFactory()
    obj = SimpleNamespace(user=user)  # sem `owner`/`created_by`

    perm = IsObjectOwner()
    request = _make_request(rf, user=user)

    assert perm.has_object_permission(request, view, obj) is True


@pytest.mark.django_db
def test_is_object_owner_denies_for_other_user(rf, view):
    user = UserFactory()
    other = UserFactory()
    obj = SimpleNamespace(owner=other)

    perm = IsObjectOwner()
    request = _make_request(rf, user=user)

    assert perm.has_object_permission(request, view, obj) is False


@pytest.mark.django_db
def test_is_object_owner_denies_anonymous(rf):
    user = UserFactory()
    obj = SimpleNamespace(owner=user)

    perm = IsObjectOwner()
    request = _make_request(rf, user=AnonymousUser())

    assert perm.has_object_permission(request, None, obj) is False


@pytest.mark.django_db
def test_is_object_owner_denies_when_no_owner_attr(rf, view):
    user = UserFactory()
    obj = SimpleNamespace(name="foo")  # nenhum dos defaults

    perm = IsObjectOwner()
    request = _make_request(rf, user=user)

    assert perm.has_object_permission(request, view, obj) is False


@pytest.mark.django_db
def test_is_object_owner_respects_view_owner_field(rf):
    user = UserFactory()
    obj = SimpleNamespace(autor=user)
    view = SimpleNamespace(owner_field="autor")

    perm = IsObjectOwner()
    request = _make_request(rf, user=user)

    assert perm.has_object_permission(request, view, obj) is True


# ---------- ReadOnly ----------


def test_readonly_grants_safe_methods(rf, view):
    perm = ReadOnly()
    for method in ("get", "head", "options"):
        request = _make_request(rf, method=method)
        assert perm.has_permission(request, view) is True, method


def test_readonly_denies_unsafe_methods(rf, view):
    perm = ReadOnly()
    for method in ("post", "put", "patch", "delete"):
        request = _make_request(rf, method=method)
        assert perm.has_permission(request, view) is False, method

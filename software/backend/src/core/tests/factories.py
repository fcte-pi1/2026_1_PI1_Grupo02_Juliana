"""Factories transversais — User. Importadas direto, sem `register()`."""
import uuid

import factory
from django.contrib.auth import get_user_model


class UserFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = get_user_model()
        django_get_or_create = ("username",)

    # UUID curto pra evitar colisão entre tests/sessões — Sequence global
    # do factory-boy não combina bem com --reuse-db.
    username = factory.LazyFunction(lambda: f"user-{uuid.uuid4().hex[:8]}")
    email = factory.LazyAttribute(lambda obj: f"{obj.username}@example.com")
    password = factory.PostGenerationMethodCall("set_password", "x")

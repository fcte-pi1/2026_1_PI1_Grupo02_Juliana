"""Factory de `ServiceCheck`. Importada direto, sem `pytest_factoryboy.register`."""
import factory

from healthcheck.models import ServiceCheck


class ServiceCheckFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = ServiceCheck

    name = factory.Sequence(lambda n: f"check-{n}")
    url = factory.Faker("url", locale="pt_BR")
    expected_status = 200
    interval_seconds = 60
    is_active = True

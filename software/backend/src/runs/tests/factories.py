"""Factories dos modelos de runs."""
import factory

from runs.models import Labirinto, Micromouse, Tentativa


class MicromouseFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Micromouse

    nome = factory.Sequence(lambda n: f"Mouse-{n}")
    algoritmo = "Flood Fill"


class LabirintoFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Labirinto

    nome = factory.Sequence(lambda n: f"Labirinto-{n}")
    dimensao = 16


class TentativaFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Tentativa

    micromouse = factory.SubFactory(MicromouseFactory)
    labirinto = factory.SubFactory(LabirintoFactory)

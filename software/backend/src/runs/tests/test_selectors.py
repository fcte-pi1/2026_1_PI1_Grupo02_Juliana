"""Tests para runs.selectors — queries puras sobre os modelos de runs."""
import uuid
from datetime import UTC, datetime

import pytest

from core.errors import NotFoundError
from runs.models import Posicao, Tentativa
from runs.selectors import (
    get_posicoes_tentativa,
    get_tentativa_by_id,
    get_tentativa_em_curso,
    get_trajetoria_tentativa,
    list_tentativas,
)
from runs.tests.factories import MicromouseFactory, TentativaFactory


@pytest.mark.django_db
def test_get_tentativa_by_id_returns_tentativa():
    tentativa = TentativaFactory()
    found = get_tentativa_by_id(tentativa_id=str(tentativa.id))
    assert found.id == tentativa.id


@pytest.mark.django_db
def test_get_tentativa_by_id_includes_related_objects():
    tentativa = TentativaFactory()
    found = get_tentativa_by_id(tentativa_id=str(tentativa.id))
    # select_related deve ter pré-carregado as FKs
    assert found.micromouse is not None
    assert found.labirinto is not None


@pytest.mark.django_db
def test_get_tentativa_by_id_raises_not_found_for_unknown_id():
    with pytest.raises(NotFoundError):
        get_tentativa_by_id(tentativa_id=str(uuid.uuid4()))


@pytest.mark.django_db
def test_list_tentativas_returns_all():
    TentativaFactory.create_batch(3)
    qs = list_tentativas()
    assert qs.count() == 3


@pytest.mark.django_db
def test_list_tentativas_empty_when_none():
    qs = list_tentativas()
    assert qs.count() == 0


@pytest.mark.django_db
def test_get_tentativa_em_curso_returns_latest_in_progress():
    mm = MicromouseFactory()
    # Cria uma em_curso e uma finalizada para o mesmo micromouse
    em_curso = TentativaFactory(micromouse=mm, status=Tentativa.Status.EM_CURSO)
    TentativaFactory(micromouse=mm, status=Tentativa.Status.FINALIZADA)

    found = get_tentativa_em_curso(micromouse_id=str(mm.id))
    assert found is not None
    assert found.id == em_curso.id


@pytest.mark.django_db
def test_get_tentativa_em_curso_returns_none_when_not_found():
    mm = MicromouseFactory()
    TentativaFactory(micromouse=mm, status=Tentativa.Status.FINALIZADA)

    found = get_tentativa_em_curso(micromouse_id=str(mm.id))
    assert found is None


@pytest.mark.django_db
def test_get_tentativa_em_curso_returns_none_for_unknown_micromouse():
    found = get_tentativa_em_curso(micromouse_id=str(uuid.uuid4()))
    assert found is None


@pytest.mark.django_db
def test_get_posicoes_tentativa_returns_ordered_by_passo():
    tentativa = TentativaFactory()
    now = datetime.now(UTC)
    Posicao.objects.create(tentativa=tentativa, coordenada_x=2, coordenada_y=2, timestamp=now, passo=2)
    Posicao.objects.create(tentativa=tentativa, coordenada_x=0, coordenada_y=0, timestamp=now, passo=0)
    Posicao.objects.create(tentativa=tentativa, coordenada_x=1, coordenada_y=1, timestamp=now, passo=1)

    posicoes = list(get_posicoes_tentativa(tentativa_id=str(tentativa.id)))
    assert [p.passo for p in posicoes] == [0, 1, 2]


@pytest.mark.django_db
def test_get_posicoes_tentativa_returns_empty_for_no_positions():
    tentativa = TentativaFactory()
    posicoes = list(get_posicoes_tentativa(tentativa_id=str(tentativa.id)))
    assert posicoes == []


@pytest.mark.django_db
def test_get_trajetoria_tentativa_returns_correct_shape():
    tentativa = TentativaFactory()
    now = datetime.now(UTC)
    Posicao.objects.create(
        tentativa=tentativa,
        coordenada_x=3, coordenada_y=4,
        timestamp=now, passo=1,
        orientacao="N", velocidade=0.3, bateria=90.0,
    )

    trajetoria = get_trajetoria_tentativa(tentativa_id=str(tentativa.id))
    assert len(trajetoria) == 1
    item = trajetoria[0]
    assert item["x"] == 3
    assert item["y"] == 4
    assert item["step"] == 1
    assert item["orientation"] == "N"
    assert item["velocity"] == 0.3
    assert item["battery"] == 90.0


@pytest.mark.django_db
def test_get_trajetoria_tentativa_returns_empty_list():
    tentativa = TentativaFactory()
    trajetoria = get_trajetoria_tentativa(tentativa_id=str(tentativa.id))
    assert trajetoria == []

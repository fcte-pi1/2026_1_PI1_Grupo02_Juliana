"""Queries puras sobre os modelos de runs. Sem regra de negócio — só leitura."""
from django.db.models import QuerySet

from core.errors import NotFoundError
from runs.models import Tentativa


def get_tentativa_by_id(*, tentativa_id: str) -> Tentativa:
    try:
        return Tentativa.objects.select_related("micromouse", "labirinto").get(
            id=tentativa_id
        )
    except Tentativa.DoesNotExist as exc:
        raise NotFoundError(f"Tentativa {tentativa_id} not found.") from exc


def list_tentativas() -> QuerySet[Tentativa]:
    return Tentativa.objects.select_related("micromouse", "labirinto").all()


def get_tentativa_em_curso(*, micromouse_id: str) -> Tentativa | None:
    return (
        Tentativa.objects.filter(
            micromouse_id=micromouse_id, status=Tentativa.Status.EM_CURSO
        )
        .order_by("-id")
        .first()
    )

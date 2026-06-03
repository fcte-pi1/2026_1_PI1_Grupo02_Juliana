"""BaseModel abstrato — todo modelo de domínio herda daqui.

PK UUID v7 (Python 3.14+) garante ordenação cronológica natural — não precisa
de index em `created_at`. Em ambientes onde `uuid.uuid7` não estiver disponível,
fallback pra `uuid.uuid4` (perde a ordenação cronológica, mantém unicidade).
"""
import uuid

from django.db import models

try:
    _uuid_default = uuid.uuid7  # Python 3.14+
except AttributeError:  # pragma: no cover
    # Fallback: perde ordenação cronológica do v7, mantém unicidade.
    _uuid_default = uuid.uuid4


class BaseModel(models.Model):
    id = models.UUIDField(primary_key=True, default=_uuid_default, editable=False)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    class Meta:
        abstract = True
        ordering = ["-id"]

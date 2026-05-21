"""Custom DRF permissions Micromouse — roles via django.contrib.auth.Group + ownership.

Convenções:
- Roles são `Group` do django.contrib.auth (sem model custom). Grupos são
  criados via migration/seed do produto. Nomes em lowercase: "admin", "gestor",
  "membro", etc.
- `HasRole` exige 1 grupo específico. `HasAnyRole` aceita lista. Ambos exigem
  user autenticado.
- `IsObjectOwner` checa atributo `owner` ou `created_by` ou `user` do objeto.
  Customizar via `owner_field` na ViewSet quando o campo tiver outro nome.
- Compor com `&` (AND) e `|` (OR) do DRF — ex:
  `IsAuthenticated & (HasRole("admin") | IsObjectOwner)`.
"""
from rest_framework.permissions import SAFE_METHODS, BasePermission

# Atributos checados em ordem por `IsObjectOwner` quando `owner_field` não é
# customizado pela view.
_DEFAULT_OWNER_ATTRS = ("owner", "created_by", "user")


def HasRole(role: str) -> type[BasePermission]:
    """Factory: retorna uma `BasePermission` que exige `role` (nome de Group).

    Por que factory em vez de `__init__`: DRF instancia cada classe de
    `permission_classes` chamando `cls()` (sem args). Se `HasRole` fosse
    classe com `__init__(self, role)`, `permission_classes = [HasRole("admin")]`
    passaria uma *instância*, e o DRF tentaria chamar `instance()` — quebra.
    Fábrica devolve uma subclasse fresca já parametrizada via closure: o
    DRF instancia normalmente e a permission carrega o role no `name`.
    """

    class _HasRole(BasePermission):
        message = f"Requires role '{role}'."

        def has_permission(self, request, view):
            user = request.user
            return bool(
                user
                and user.is_authenticated
                and user.groups.filter(name=role).exists()
            )

    _HasRole.__name__ = f"HasRole_{role}"
    _HasRole.__qualname__ = _HasRole.__name__
    return _HasRole


def HasAnyRole(*roles: str) -> type[BasePermission]:
    """Factory: retorna `BasePermission` que aceita qualquer role da lista.

    Mesma razão de design da `HasRole` — DRF precisa de classe, não instância.
    """
    roles_tuple = tuple(roles)

    class _HasAnyRole(BasePermission):
        message = f"Requires one of roles: {', '.join(roles_tuple)}."

        def has_permission(self, request, view):
            user = request.user
            if not (user and user.is_authenticated):
                return False
            if not roles_tuple:
                return False
            return user.groups.filter(name__in=roles_tuple).exists()

    _HasAnyRole.__name__ = f"HasAnyRole_{'_'.join(roles_tuple) or 'empty'}"
    _HasAnyRole.__qualname__ = _HasAnyRole.__name__
    return _HasAnyRole


class IsObjectOwner(BasePermission):
    """Permite acesso ao objeto se o user for o "dono".

    Procura, em ordem:
    1. `view.owner_field` (string explícita na ViewSet).
    2. `obj.owner`, `obj.created_by`, `obj.user` (defaults).

    `has_permission` retorna True (filtro fica em `has_object_permission`)
    para não quebrar list endpoints — combine com `IsAuthenticated` para
    exigir login no nível do request.
    """

    message = "You do not own this object."

    def has_permission(self, request, view):
        return True

    def has_object_permission(self, request, view, obj):
        user = request.user
        if not (user and user.is_authenticated):
            return False

        custom_field = getattr(view, "owner_field", None)
        candidates = (custom_field,) if custom_field else _DEFAULT_OWNER_ATTRS

        for attr in candidates:
            if attr and hasattr(obj, attr):
                return getattr(obj, attr) == user
        return False


class ReadOnly(BasePermission):
    """Permite apenas métodos seguros (GET, HEAD, OPTIONS).

    Útil para compor: `IsAuthenticated & (HasRole("admin") | ReadOnly)` —
    todos autenticados leem; só admin escreve.
    """

    message = "Read-only endpoint for this role."

    def has_permission(self, request, view):
        return request.method in SAFE_METHODS

    def has_object_permission(self, request, view, obj):
        return request.method in SAFE_METHODS

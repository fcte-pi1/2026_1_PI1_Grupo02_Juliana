# software/

Template base de software do Micromouse PI1 Grupo 02. Backend + frontend + orquestração via Docker Compose, pronto pra rodar local.

## Stack

- **Backend:** Django 6 + DRF + JWT + Celery/Redis + PostgreSQL 18 + MinIO, Python 3.14, `uv`, Ruff.
- **Frontend:** Vite + React 19 + TypeScript strict + Mantine v9 + TanStack Query v5 + React Router v7, pnpm.
- **Orquestração:** Docker Compose v2 + `Makefile` como interface única.

Stack ratificada na reunião de 29/04/2026 (Python + PostgreSQL no backend, React no frontend).

## Quickstart

Pré-requisitos: Docker + Docker Compose, `make`. Tudo o resto roda dentro dos containers.

```bash
cd software

# 1. Configura env (defaults locais já funcionam)
cp .env.example .env

# 2. Sobe tudo
make build
make up
make migrate
make seed         # cria groups (admin/gestor/membro) + superuser admin/admin123

# 3. Acesse
# Backend:        http://localhost:8000/api/v1/health/
# Frontend:       http://localhost:5173
# Django admin:   http://localhost:8000/admin/
# MinIO console:  http://localhost:9001
```

Derrubar: `make down`. Apagar volumes (DB + MinIO): `make clear`.

## Estrutura

```
software/
├── Makefile                          # única interface de comandos
├── docker/                           # compose.yml + Dockerfiles
├── backend/
│   ├── pyproject.toml                # uv + ruff
│   ├── conftest.py
│   ├── manage.py
│   ├── config/
│   │   ├── celery_app.py
│   │   ├── urls.py
│   │   └── settings/{base,local,test,production}.py
│   └── src/
│       ├── core/                     # BaseModel, errors, envelope, JWT, middleware
│       ├── integrations/             # clients externos (LLM, S3, etc)
│       └── healthcheck/              # APP EXEMPLO — apagar quando não precisar
└── frontend/
    ├── package.json                  # pnpm
    ├── vite.config.ts
    └── src/
        ├── main.tsx
        ├── router.tsx
        ├── components/{atoms,molecules,templates}/
        ├── lib/
        └── domains/
            ├── auth/                 # login + JWT (todo produto tem)
            └── healthcheck/          # DOMAIN EXEMPLO — apagar quando não precisar
```

## Comandos (Makefile)

| Comando | O que faz |
|---|---|
| `make up` / `make down` | sobe/derruba todos os serviços |
| `make build` | rebuild de imagens (no-cache) |
| `make logs` | tail de todos os logs |
| `make migrate` / `make migrations` | aplica / cria migrations |
| `make shell` | `shell_plus` no backend |
| `make superuser` | cria superuser Django (interativo) |
| `make seed` | cria groups default (admin/gestor/membro) + superuser admin/admin123 (idempotente) |
| `make test` / `make test-fast` | suite verbose / paralelo (`-n auto --reuse-db`) |
| `make lint` / `make fmt` | ruff check / format |
| `make restart-celery` | restart só do worker |
| `make frontend-lint` / `make frontend-typecheck` | lint / typecheck do frontend |
| `make bash` / `make frontend-bash` | shell dentro do container |
| `make clear` | `down -v --remove-orphans` (apaga volumes) |

## App exemplo: healthcheck

Mostra o caminho completo de uma feature: model → migration → selector → use case → view → serializer → tests + frontend api → types → hooks → page.

`ServiceCheck` model com campos `name, url, expected_status, interval_seconds, is_active, last_checked_at, last_status`. Use case `RunCheck` faz HTTP GET via httpx. Endpoint `POST /api/v1/healthcheck/checks/{id}/run/` dispara manualmente. Frontend lista os checks e dispara via botão.

**Apagar quando não precisar mais:**

```bash
rm -rf backend/src/healthcheck frontend/src/domains/healthcheck
# remover do INSTALLED_APPS, urls.py, router.tsx
```

## Convenções

- **Backend:** lógica em `use_cases/` (1 classe, 1 método `execute`, ≤30 linhas), queries em `selectors.py` (read-only, kw-only), helpers em `services/` (stateless), tasks Celery thin em `tasks.py`. Views nunca capturam erro: `core.errors.custom_exception_handler` é global. Toda response passa pelo `EnvelopeRenderer`.
- **Frontend:** atomic design (`atoms`/`molecules`/`templates`) + `domains/` por intenção do usuário. Cross-domain import proibido. `apiClient` único em `domains/auth/api/client.ts`. TanStack Query encapsulado em hooks por domain. Mantine v9 com `rem()` e theme tokens.

## Próximos passos

1. Apaga o app `healthcheck` quando o domínio real começar.
2. Adiciona apps de domínio seguindo o padrão do `healthcheck` como referência viva.
3. Define endpoints de comunicação com firmware (`/api/v1/missions/`, `/api/v1/runs/`, etc).

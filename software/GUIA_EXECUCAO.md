# Guia de execucao local

Este projeto roda localmente com Docker Compose. O Compose sobe backend, frontend,
PostgreSQL, Redis, MinIO, Celery worker e Celery beat.

## Pre-requisitos

Obrigatorios:

- Docker Desktop
- Docker Compose v2, disponivel como `docker compose`

Opcionais no Windows:

- `make`, para usar os atalhos do `Makefile`
- Node.js/Corepack/pnpm, apenas se quiser rodar o frontend fora do Docker
- Python/uv, apenas se quiser rodar o backend fora do Docker

No fluxo recomendado, `pnpm` e `uv` sao instalados dentro dos containers.

## Primeira execucao

Abra o PowerShell na raiz do repositorio e entre na pasta de software:

```powershell
cd C:\Documentos\GitHub\2026_1_PI1_Grupo02_Juliana\software
```

Crie o arquivo de variaveis de ambiente, se ainda nao existir:

```powershell
Copy-Item .env.example .env
```

Construa as imagens:

```powershell
docker compose --env-file .env -f docker/compose.yml build
```

Suba todos os servicos:

```powershell
docker compose --env-file .env -f docker/compose.yml up -d
```

Aplique as migrations do banco:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run python manage.py migrate
```

Rode o seed inicial:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run python manage.py seed
```

O seed cria os grupos `admin`, `gestor` e `membro`, alem do superusuario:

- usuario: `admin`
- senha: `admin123`

## URLs

- Frontend: http://localhost:5173
- Backend healthcheck: http://localhost:8000/api/v1/health/
- Django admin: http://localhost:8000/admin/
- MinIO console: http://localhost:9001
- MinIO API: http://localhost:9000

## Comandos uteis

Ver status dos containers:

```powershell
docker compose --env-file .env -f docker/compose.yml ps
```

Ver logs de todos os servicos:

```powershell
docker compose --env-file .env -f docker/compose.yml logs -f
```

Ver logs de um servico especifico:

```powershell
docker compose --env-file .env -f docker/compose.yml logs -f backend
docker compose --env-file .env -f docker/compose.yml logs -f frontend
docker compose --env-file .env -f docker/compose.yml logs -f postgres
```

Parar os containers sem apagar dados:

```powershell
docker compose --env-file .env -f docker/compose.yml down
```

Parar e apagar volumes, incluindo banco e MinIO:

```powershell
docker compose --env-file .env -f docker/compose.yml down -v --remove-orphans
```

## Backend

Executar migrations:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run python manage.py migrate
```

Criar migrations:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run python manage.py makemigrations
```

Criar superusuario manualmente:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run python manage.py createsuperuser
```

Abrir shell Django:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run python manage.py shell_plus
```

Rodar testes:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run pytest -v
```

Rodar lint:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run ruff check .
```

Formatar backend:

```powershell
docker compose --env-file .env -f docker/compose.yml exec backend uv run ruff format .
```

## Banco de dados

O PostgreSQL nao expoe a porta `5432` para o host por padrao. Para acessar via
`psql` dentro do container:

```powershell
docker compose --env-file .env -f docker/compose.yml exec postgres psql -U micromouse micromouse
```

As credenciais locais padrao ficam em `.env`:

- banco: `micromouse`
- usuario: `micromouse`
- senha: `micromouse`
- host interno: `postgres`
- porta interna: `5432`

## Frontend

O frontend roda com Vite dentro do container:

```powershell
docker compose --env-file .env -f docker/compose.yml exec frontend pnpm dev --host 0.0.0.0
```

Normalmente nao e preciso rodar esse comando manualmente, porque o servico
`frontend` ja sobe com `docker compose up -d`.

Rodar lint:

```powershell
docker compose --env-file .env -f docker/compose.yml exec frontend pnpm lint
```

Rodar typecheck:

```powershell
docker compose --env-file .env -f docker/compose.yml exec frontend pnpm typecheck
```

## Equivalentes do Makefile

Se voce instalar `make`, estes comandos do `Makefile` ficam disponiveis dentro
da pasta `software`:

```powershell
make build
make up
make migrate
make seed
make down
make logs
```

Nesta maquina, o `make` nao estava instalado, entao o fluxo acima usa os comandos
`docker compose` completos.


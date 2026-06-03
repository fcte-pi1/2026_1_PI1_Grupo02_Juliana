COMPOSE = docker compose --env-file .env -f docker/compose.yml

.PHONY: help up down restart restart-celery build rebuild logs ps \
        migrate migrations shell superuser seed simulate \
        test test-fast lint fmt \
        frontend-lint frontend-typecheck \
        clear bash frontend-bash

help:
	@echo "Targets disponíveis:"
	@echo "  up               - sobe a stack em background"
	@echo "  down             - derruba a stack"
	@echo "  restart          - reinicia todos os services"
	@echo "  restart-celery   - reinicia só o celery_worker"
	@echo "  build            - build das imagens (sem cache)"
	@echo "  rebuild          - down + build + up"
	@echo "  logs             - tail -f de todos os logs"
	@echo "  ps               - status dos services"
	@echo "  migrate          - aplica migrations"
	@echo "  migrations       - gera novas migrations"
	@echo "  shell            - shell_plus no backend"
	@echo "  superuser        - cria superuser Django"
	@echo "  seed             - seed inicial (manage.py seed)"
	@echo "  test             - pytest verbose"
	@echo "  test-fast        - pytest paralelo + reuse-db"
	@echo "  lint             - ruff check"
	@echo "  fmt              - ruff format"
	@echo "  frontend-lint    - pnpm lint"
	@echo "  frontend-typecheck - pnpm typecheck"
	@echo "  bash             - bash dentro do backend"
	@echo "  frontend-bash    - sh dentro do frontend"
	@echo "  clear            - down -v --remove-orphans (apaga volumes)"

# ─── Lifecycle ────────────────────────────────────────────────────────────────

up:
	$(COMPOSE) up -d

down:
	$(COMPOSE) down

restart:
	$(COMPOSE) restart

restart-celery:
	$(COMPOSE) restart celery_worker

build:
	$(COMPOSE) build --no-cache

rebuild: down build up

logs:
	$(COMPOSE) logs -f

ps:
	$(COMPOSE) ps

# ─── Django ───────────────────────────────────────────────────────────────────

migrate:
	$(COMPOSE) exec backend uv run python manage.py migrate

migrations:
	$(COMPOSE) exec backend uv run python manage.py makemigrations

shell:
	$(COMPOSE) exec backend uv run python manage.py shell_plus

superuser:
	$(COMPOSE) exec backend uv run python manage.py createsuperuser

seed:
	$(COMPOSE) exec backend uv run python manage.py seed

simulate:
	$(COMPOSE) exec backend uv run python manage.py mqtt_simulate

bash:
	$(COMPOSE) exec backend bash

# ─── Quality (backend) ────────────────────────────────────────────────────────

test:
	$(COMPOSE) exec backend uv run pytest -v

test-fast:
	$(COMPOSE) exec backend uv run pytest -n auto --reuse-db -q

lint:
	$(COMPOSE) exec backend uv run ruff check .

fmt:
	$(COMPOSE) exec backend uv run ruff format .

# ─── Quality (frontend) ───────────────────────────────────────────────────────

frontend-lint:
	$(COMPOSE) exec frontend pnpm lint

frontend-typecheck:
	$(COMPOSE) exec frontend pnpm typecheck

frontend-bash:
	$(COMPOSE) exec frontend sh

# ─── Nuke ─────────────────────────────────────────────────────────────────────

clear:
	$(COMPOSE) down -v --remove-orphans

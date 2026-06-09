"""Seed inicial para desenvolvimento local.

Cria:
- Groups default (`admin`, `gestor`, `membro`) usados por `core.permissions.HasRole`.
- Superuser `admin` / `admin123` (override via env DJANGO_SEED_ADMIN_USER / _PASS / _EMAIL).

Idempotente: pode rodar várias vezes. Recusa rodar em produção
(DJANGO_SETTINGS_MODULE terminando em `.production`) salvo `--force`.
"""
from __future__ import annotations

import os

from django.contrib.auth import get_user_model
from django.contrib.auth.models import Group
from django.core.management.base import BaseCommand, CommandError

DEFAULT_GROUPS = ("admin", "gestor", "membro")


class Command(BaseCommand):
    help = "Cria groups default e um superuser admin (apenas dev/local)."

    def add_arguments(self, parser):
        parser.add_argument(
            "--force",
            action="store_true",
            help="Permite rodar mesmo com settings de produção (use com cuidado).",
        )

    def handle(self, *args, **options):
        settings_module = os.environ.get("DJANGO_SETTINGS_MODULE", "")
        if settings_module.endswith(".production") and not options["force"]:
            raise CommandError(
                "Recusando rodar seed em produção. Use --force se realmente quiser."
            )

        for name in DEFAULT_GROUPS:
            _, created = Group.objects.get_or_create(name=name)
            self.stdout.write(
                f"  group {name}: {'criado' if created else 'já existia'}"
            )

        User = get_user_model()
        username = os.environ.get("DJANGO_SEED_ADMIN_USER", "admin")
        password = os.environ.get("DJANGO_SEED_ADMIN_PASS", "admin123")
        email = os.environ.get("DJANGO_SEED_ADMIN_EMAIL", "admin@example.com")

        user, created = User.objects.get_or_create(
            username=username,
            defaults={"email": email, "is_staff": True, "is_superuser": True},
        )
        if created:
            user.set_password(password)
            user.save()
            user.groups.add(Group.objects.get(name="admin"))
            self.stdout.write(
                self.style.SUCCESS(f"  superuser {username} criado")
            )
        else:
            self.stdout.write(f"  superuser {username}: já existia (senha não tocada)")

        self.stdout.write(self.style.SUCCESS("seed concluído"))

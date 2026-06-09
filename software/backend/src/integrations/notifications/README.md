# `integrations/notifications/`

Placeholder. Aqui virão os clients de canais de notificação:

- `whatsapp_client.py` — uazapi (envio de mensagem/áudio via instância WA).
- `email_client.py` — SES ou SMTP (transacional).
- `slack_client.py` — webhook ou bot token.

Convenção igual a `fetcher/`:

- `base.py` define `Notifier` (interface) + `NotificationError`.
- `<provider>_client.py` implementa a interface.
- `factory.py` se houver múltiplos providers da mesma categoria.
- Lib do SDK do provider entra opt-in via `uv add` no produto que usar — import lazy dentro do método.

Credenciais sempre via `decouple.config`. Nada hardcoded.

"""Esqueleto demonstrando padrão Micromouse de LLM client com prompt caching.

Ative com `uv add anthropic` no produto que precisar — a lib NÃO está no
pyproject default do template porque LLM é opt-in por produto.

Padrões importantes:
- import lazy do `anthropic` dentro do método (não quebra import estático).
- prompt caching ativado por default no system block (`cache_control: ephemeral`).
- api_key via decouple.config quando não passada.
"""

from decouple import config


class AnthropicClient:
    def __init__(
        self,
        *,
        api_key: str | None = None,
        model: str = "claude-sonnet-4-6",
    ):
        self._api_key = api_key or config("ANTHROPIC_API_KEY", default=None)
        self._model = model
        self._client = None  # lazy init no primeiro complete()

    def _get_client(self):
        if self._client is None:
            # Import lazy — anthropic NÃO está no pyproject default.
            # Ative com `uv add anthropic` no produto que consumir esse client.
            from anthropic import Anthropic

            if not self._api_key:
                raise RuntimeError(
                    "ANTHROPIC_API_KEY não configurada (env ou api_key=...)."
                )
            self._client = Anthropic(api_key=self._api_key)
        return self._client

    def complete(
        self,
        *,
        system: str,
        messages: list[dict],
        cache_system: bool = True,
        max_tokens: int = 4096,
    ) -> str:
        """Chama messages.create e devolve o texto da primeira content block.

        Quando `cache_system=True`, envolve o system prompt num bloco com
        `cache_control: {"type": "ephemeral"}` pra ativar prompt caching no
        provider — dramaticamente reduz custo/latência em chamadas repetidas
        com o mesmo system.
        """
        client = self._get_client()

        kwargs: dict = {
            "model": self._model,
            "max_tokens": max_tokens,
            "messages": messages,
        }
        if cache_system:
            kwargs["system"] = [
                {
                    "type": "text",
                    "text": system,
                    "cache_control": {"type": "ephemeral"},
                }
            ]
        else:
            kwargs["system"] = system

        response = client.messages.create(**kwargs)
        return response.content[0].text

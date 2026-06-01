# Guia de Telemetria e Mapeamento do Labirinto

Documenta a feature de **mapeamento do labirinto em tempo real**: o que foi
construído, como executar e testar, o contrato de dados e os próximos passos para
conectar o robô real.

A arquitetura-alvo está especificada em
`docs/relatorio/editaveis/06_projetoconceitual.tex` (visões 4+1). Este guia
descreve a **fatia vertical** já implementada dela.

---

## 1. Visão geral do fluxo

```
[Firmware / Simulador]
        │  publica MQTT (QoS 1)
        ▼
   [Mosquitto]  micromouse/<id>/{telemetria,evento,status}
        │  subscribe
        ▼
[mqtt_subscriber]  (processo dedicado)
        │  valida → use case → persiste (Postgres)
        │  publica snapshot
        ▼
   [Redis pub/sub]  canal telemetry:<tentativa_id>
        │  subscribe
        ▼
[Django web]  GET /api/v1/runs/<id>/stream/  (SSE)
        │  EventSource
        ▼
[Dashboard React]  MazeCanvas desenha N×N em tempo real
```

Decisões-chave (confirmadas / herdadas do documento):

- **Transporte:** MQTT (Mosquitto em Docker). Robô é *publisher*, backend é *subscriber*.
- **Quem infere parede:** o **firmware** manda a *matriz delta* já calculada (flags
  N/S/L/O por célula), não dados crus de sensor. O backend só **aplica e persiste**.
- **Backend autoritativo:** monta o estado completo do labirinto a partir dos deltas.
- **Push ao dashboard:** **SSE** (Server-Sent Events) — roda no processo web, sem
  Django Channels. Atende RNF04 (<1s) e RNF05 (~500ms).
- **Dimensão do labirinto:** configurável por `Labirinto.dimensao` (4/8/16…).
- **Comandos (RF22):** REST `POST .../comando/`. Hoje o `start` dispara o simulador
  (task Celery) como stand-in do robô; o `stop` sinaliza parada via Redis.

---

## 2. O que mudou (changelog)

### 2.1 Backend — app novo `runs` (`backend/src/runs/`)
Segue o padrão do app `healthcheck` (model → selectors → use_cases → api).

| Arquivo | Papel |
|---|---|
| `models.py` | `Micromouse`, `Labirinto` (`dimensao`), `Tentativa` (`maze` JSONField, `pose`, `status`), `Posicao` (trajetória) — alinhados ao MER |
| `schemas.py` | Schemas pydantic dos pacotes MQTT (`TelemetriaPayload`, `EventoPayload`) — validação na borda |
| `selectors.py` | Queries read-only (`get_tentativa_by_id`, `list_tentativas`) |
| `services/maze.py` | `apply_delta` — aplica a matriz delta ao estado do labirinto (puro) |
| `services/snapshot.py` | `build_snapshot` — monta o payload servido ao dashboard |
| `services/simulator.py` | `run_simulation` — corrida simulada (stand-in do firmware) |
| `use_cases/persistir_telemetria.py` | `PersistirTelemetria` — aplica delta + cria `Posicao` + métricas |
| `use_cases/registrar_evento.py` | `RegistrarEvento` — início/colisão/reparo/desafio_cumprido |
| `realtime.py` | Ponte Redis pub/sub (`publish_snapshot` / `subscribe`) entre subscriber e SSE |
| `tasks.py` | `simular_corrida` (Celery) + `parar_corrida` (flag de stop no Redis) |
| `api/views.py` | `TentativaViewSet` (list/detail/`snapshot`/`comando`) |
| `api/sse.py` | `telemetry_stream` — view Django plain (SSE, fora do DRF) |
| `api/serializers.py`, `api/urls.py` | Serializers + rotas (router + rota SSE) |
| `management/commands/mqtt_subscribe.py` | Processo subscriber MQTT do backend |
| `management/commands/mqtt_simulate.py` | Simulador via CLI |
| `tests/` | `factories`, `test_use_cases`, `test_api` (8 testes) |

### 2.2 Backend — integração MQTT
- `backend/src/integrations/mqtt/client.py` — wrapper paho + helpers de tópico
  (`topic_telemetria`, `topic_evento`, `topic_status`, `topic_comando`).

### 2.3 Backend — config
- `pyproject.toml` / `uv.lock`: adicionada dep **`paho-mqtt`**.
- `config/settings/base.py`: vars `MQTT_*` + `runs` em `INSTALLED_APPS`.
- `config/urls.py`: monta `api/v1/runs/`.

### 2.4 Frontend — domínio `runs` (`frontend/src/domains/runs/`)
| Arquivo | Papel |
|---|---|
| `types/index.ts` | `RunSnapshot`, `MazeState`, `Pose`, `Tentativa` (espelham os serializers) |
| `api/index.ts` | `fetchTentativas`, `fetchSnapshot`, `enviarComando` |
| `hooks/useTentativas.ts` | lista de corridas (TanStack Query) |
| `hooks/useRunSnapshot.ts` | snapshot inicial (REST) |
| `hooks/useTelemetryStream.ts` | **EventSource/SSE** ao vivo (JWT por query param) |
| `components/MazeCanvas/MazeCanvas.tsx` | render N×N do labirinto a partir do estado real |

### 2.5 Frontend — outros
- `pages/DashboardPage.tsx`: layout rico do protótipo **preservado**, mas o
  **labirinto, a pose e as métricas principais agora vêm de dados reais** (REST + SSE);
  botões **Iniciar/Parar run** ligados ao endpoint `comando`. Sensores/IMU/sparklines/
  eventos seguem simulados (cosméticos) até o firmware enviá-los.
- `router.tsx`: dashboard vira rota **full-screen** (`/`) com guarda `RequireAuth`
  (sem o `AppShellTemplate`); páginas com chrome padrão movidas para `/app/*`.
- `domains/auth/components/RequireAuth/RequireAuth.tsx`: guarda de rota sem chrome.
- `domains/auth/api/config.ts`: **correção do login** — `||` no lugar de `??`, para a
  `VITE_API_BASE_URL` vazia cair no default `/api` (antes o login ia para `/v1/token/`
  fora do proxy e falhava).

### 2.6 Infra
- `docker/compose.yml`: serviços **`mosquitto`** (broker) e **`mqtt_subscriber`**
  (subscriber) + volume `mosquittodata`.
- `docker/mosquitto/mosquitto.conf`: config mínima do broker (anônimo em dev).
- `.env.example`: `MQTT_HOST/PORT/BASE_TOPIC/USERNAME/PASSWORD`.
- `Makefile`: alvo `make simulate`.

---

## 3. Guia de execução

A partir de `software/`:

```bash
cp .env.example .env          # 1ª vez (agora inclui as vars MQTT_*)
make build                    # rebuild (instala paho-mqtt)
make up                       # sobe tudo, incl. mosquitto + mqtt_subscriber
make migrate                  # aplica a migration do app runs
make seed                     # superuser admin/admin123 (se ainda não fez)
```

Suba/confira os serviços com `make ps` — devem aparecer `mosquitto` e `mqtt_subscriber`.

### Ver o mapeamento ao vivo
1. Abra `http://localhost:5173`, login `admin` / `admin123`.
2. Rode uma corrida simulada:
   ```bash
   make simulate
   # ou, mais lenta para ver ao vivo:
   docker compose --env-file .env -f docker/compose.yml \
     exec backend uv run python manage.py mqtt_simulate --hz 1 --steps 40
   ```
   O comando imprime o `run_id`. Abra `http://localhost:5173/?run=<run_id>`.
3. O labirinto se desenha em tempo real. Os botões **Iniciar run** / **Parar run**
   no topo disparam/abortam a corrida da tentativa selecionada.

### URLs
- Dashboard: http://localhost:5173
- API runs: http://localhost:8000/api/v1/runs/tentativas/
- SSE: http://localhost:8000/api/v1/runs/tentativas/&lt;id&gt;/stream/?token=&lt;jwt&gt;
- Admin: http://localhost:8000/admin/ (modelos em **Runs**)

---

## 4. Como testar (do mais leve ao completo)

**A. Só ver um labirinto na tela (sem MQTT):** crie uma `Tentativa` com `maze`:
```bash
docker compose --env-file .env -f docker/compose.yml exec backend \
  uv run python manage.py shell -c "
from runs.models import *
mm=Micromouse.objects.create(nome='T'); lab=Labirinto.objects.create(nome='L',dimensao=16)
t=Tentativa.objects.create(micromouse=mm,labirinto=lab,
  maze={'0,0':{'n':True,'s':False,'e':False,'w':True}}, pose={'x':0,'y':0,'heading':'N'})
print('abra http://localhost:5173/?run='+str(t.id))"
```

**B. Um pacote de ingestão (MQTT):**
```bash
docker compose --env-file .env -f docker/compose.yml exec mosquitto \
  mosquitto_pub -t "micromouse/<micromouse_id>/telemetria" \
  -m '{"ts":"2026-05-24T00:00:00Z","run_id":"<tentativa_id>","pose":{"x":2,"y":0,"heading":"E"},"maze_delta":[{"x":2,"y":0,"walls":{"n":true,"s":false,"e":false,"w":false}}],"speed":0.3,"battery":88}'
```

**C. Endpoints REST (curl):** obter token em `/api/v1/token/`, depois
`GET .../snapshot/`, `POST .../comando/` com `{"acao":"start"|"stop"}`.

**D. Testes automatizados (sem subir MQTT):**
```bash
make test                 # backend (inclui app runs)
make frontend-lint
make frontend-typecheck
```

---

## 5. Contrato de dados (MQTT)

Base topic configurável (`MQTT_BASE_TOPIC`, default `micromouse`).

### `micromouse/<id>/telemetria` — QoS 1, não-retained, ~10–30 Hz
```json
{
  "ts": "ISO-8601",
  "run_id": "<uuid da Tentativa>",
  "pose": { "x": 8, "y": 10, "heading": "N|S|E|W" },
  "maze_delta": [
    { "x": 8, "y": 10, "walls": { "n": false, "s": true, "e": true, "w": false } }
  ],
  "speed": 0.34,
  "battery": 78,
  "voltage": 7.42
}
```
`maze_delta` = lista de células cujo estado de parede mudou (o firmware manda a
célula consolidada, não diffs de parede).

### `micromouse/<id>/evento` — QoS 1, retained
```json
{ "ts": "ISO-8601", "run_id": "<uuid>", "type": "inicio|colisao|reparo|desafio_cumprido", "detail": "" }
```

### `micromouse/<id>/status` — QoS 1, retained (LWT)
```json
{ "status": "online|offline" }
```

### Snapshot enviado ao browser (SSE)
```json
{ "tentativa_id", "dimensao", "status", "pose", "maze", "explored",
  "velocidade_media", "consumo_bateria", "speed", "battery", "voltage", "ts" }
```

> O schema acima é uma **proposta**. O documento fixa os tópicos e o conceito de
> "matriz delta", mas não os campos exatos — **ratificar com a equipe de firmware**.

---

## 6. Próximos passos — conectar o rato real

Hoje o **simulador** faz o papel do firmware. Para plugar o robô:

### 6.1 No firmware (Raspberry Pi Pico W)
1. Conectar ao Wi-Fi e ao broker MQTT (`MQTT_HOST` do servidor de bancada, porta 1883).
2. Definir o **`<id>` do robô** no tópico (ver 6.3) e publicar:
   - `telemetria` a 10–30 Hz com o JSON da Seção 5 (pose + `maze_delta` + bateria/velocidade);
   - `evento` (`inicio` ao começar, `desafio_cumprido` ao chegar ao centro);
   - `status` com **Last Will and Testament** (`offline`) configurado no connect.
3. Assinar `micromouse/<id>/comando` e reagir a `{"acao":"start"|"stop"}` (ver 6.4).

### 6.2 No backend — desligar o simulador
- Em `runs/api/views.py::comando`, hoje o `start` chama `simular_corrida.delay(...)`.
  Trocar por **publicar em `topic_comando`** (helper já existe em
  `integrations/mqtt/client.py`) para o robô real receber. Usar um publish que
  garanta entrega (`loop_start()`/flush, ou `paho.mqtt.publish.single`).
- O simulador (`mqtt_simulate` / `simular_corrida`) pode continuar existindo para
  testes/demo, mas deixa de ser acionado pelos botões.

### 6.3 Resolver o `<id>` e o `run_id` (principal ponto de integração)
Duas identidades precisam ser acordadas:
- **`<id>` no tópico**: id do robô. Sugestão: um identificador fixo gravado no
  firmware, mapeado a um registro `Micromouse` no backend.
- **`run_id` no payload**: hoje é o UUID de uma `Tentativa`. O robô **não sabe** esse
  UUID. Opções a decidir:
  - (a) O backend **cria a `Tentativa` ao receber o `inicio`** (ou no comando `start`)
    e o robô usa um id de corrida próprio que o backend mapeia; **ou**
  - (b) O backend cria a `Tentativa`, publica o `run_id` no `comando` de `start`, e o
    robô ecoa esse `run_id` na telemetria.
  Recomenda-se (b): casa com o fluxo "operador clica Iniciar → backend cria run →
  manda start+run_id → robô responde".

### 6.4 Comando remoto ponta a ponta
- Operador clica **Iniciar run** → `POST .../comando/ {"acao":"start"}` → backend cria
  `Tentativa`, publica `start`+`run_id` em `comando` → robô começa e publica telemetria.
- **Parar run** → publica `stop` no `comando` → robô para.

### 6.5 Segurança / produção
- Habilitar **auth no Mosquitto** (`MQTT_USERNAME`/`MQTT_PASSWORD`, já lidos pelo
  client) e remover `allow_anonymous`. Avaliar TLS no broker.
- SSE autentica por **JWT em query param** (limitação do `EventSource`); em produção,
  considerar cookie httpOnly ou um token de short-lived específico para o stream.
- Servir o web com Gunicorn usando workers que suportem streaming (gthread) — a view
  SSE segura um worker por conexão aberta.

### 6.6 Pontos a ratificar com a equipe (divergem/estendem o documento)
- **Subscriber como management command** (processo `mqtt_subscriber`), não Celery
  worker como o `.tex` cita — loop MQTT persistente não encaixa em task Celery.
- **Push por SSE**, não WebSocket.
- **`Tentativa.maze` como JSONField** — extensão ao MER (que só modelava `Posicao`).
- **Schema JSON dos pacotes** (Seção 5) é proposta — confirmar com firmware.
- **Nomenclatura dos modelos**: o documento usa dois conjuntos (`Run/Leitura/Decisao`
  na visão de implementação vs `Micromouse/Labirinto/Tentativa/Posicao` no MER). O
  código segue o **MER**.

---

## 7. Referência rápida de comandos

```bash
make up / make down            # sobe / derruba a stack
make migrate / make migrations # aplica / gera migrations
make simulate                  # corrida simulada (cria tentativa)
make test                      # testes backend
make frontend-lint / -typecheck
make logs                      # logs de tudo
docker compose --env-file .env -f docker/compose.yml logs -f mqtt_subscriber
docker compose --env-file .env -f docker/compose.yml restart celery_worker  # após mexer em tasks.py
```

# Roadmap PI1 — Micromouse Grupo 02 · 2026.1

> **Mapa-mestre do projeto até a APT (julho/2026).** Cruza o template oficial (`fcte-pi1/template`), as alterações do Hilmer 22/04, o estado real do grupo (varredura WA 22–29/04) e a estrutura de avaliação da disciplina.
> **Como usar:** abrir antes de reunião de líderes; atualizar status de US ao longo da semana; trazer a foto pro relatório PC1/PC2.
> **Versão:** v1 — 2026-04-29. Substitui `_archive/eap-v1-pre-roadmap-2026-04-29.md`.

---

## 0 — TL;DR

- **Onde estamos:** semana 3 começando, **12 dias corridos até PC1 (11/mai)**. Labirinto físico ✅, repo criado ✅, GitPages ✅, lista de componentes fechada ✅. Faltam 3 entregas estruturantes da PC1 (LaTeX iniciado, EAP no formato Hilmer, divisão SW por camadas) e 4 decisões abertas (1S/2S, stack frontend, simulador na PC1, membros HW/EN/ES nomeados).
- **🔴 Risco-fogo:** o `main` do nosso repo está **desatualizado em relação ao template** — Hilmer fez 3 commits em 22/04 que alteram a subseção 6.5 (Software). PR de sync já preparado e empurrado: branch `sync/template-hilmer-22-04`. **Falta abrir o PR no GitHub e mergear.**
- **Próxima ação:** levar este roadmap pra reunião de hoje, distribuir donos das 4 camadas SW (proposta no § 5), confirmar com Hilmer/Juliana o formato exato da EAP (§ 4) e fechar 1S vs 2S com Guilherme Quirino.

---

## 1 — Risco-fogo: template desatualizado

### 1.1 O que aconteceu
Em **22/04/2026**, o prof. **Hilmer Neri** (`hneri`, líder dos profs de Software na FGA) fez 3 commits em `github.com/fcte-pi1/template@main` reescrevendo parte do capítulo 6 (Projeto Conceitual) do relatório.

Nosso repo (`fcte-pi1/2026_1_PI1_Grupo02_Juliana`) foi criado em **15/04** como cópia do template — antes desses commits. O `main` ainda está com a versão antiga.

**Se o grupo escrever o relatório PC1 a partir do `06_projetoconceitual.tex` que está hoje no nosso repo, vamos preencher requisitos que não existem mais e deixar de preencher os que entraram.**

### 1.2 Status do PR
- **Branch já preparada e pushada:** `sync/template-hilmer-22-04` em `fcte-pi1/2026_1_PI1_Grupo02_Juliana`.
- 3 commits aplicados via cherry-pick, autoria `hneri` preservada.
- Conflito-zero com `main` (verificado: o Initial Commit do Giovanni 15/04 era cópia byte-a-byte do template em 15/04 — ancestral comum limpo).
- **URL pra abrir o PR:** `https://github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana/pull/new/sync/template-hilmer-22-04`.
- **Texto sugerido do PR** em `_archive/pr-template-sync-text.md`.

### 1.3 Pré-condições antes de mergear
1. **Confirmar com o time** que ninguém está editando o LaTeX localmente fora do repo (Overleaf descolado, cópia local não-commitada). O `main` só tem o Initial Commit, então é improvável — mas vale o ping no chat geral antes do merge.
2. **Verificar branch protection** no `main` — se exigir review, precisa que pelo menos 1 outro membro aprove.
3. **Avisar Cairo + Georgia** (que sinalizou a mudança no chat 27/04) antes do merge, pra eles puxarem depois.

---

## 2 — O que mudou no template (Hilmer · 22/04)

### 2.1 Arquivos atingidos

| Arquivo | Tipo | Linhas |
|---|---|---|
| `docs/relatorio/editaveis/06_projetoconceitual.tex` | semântico — subseção 6.5 reescrita + ajuste | +32 / −41 |
| `docs/relatorio/fixos/pacotes.tex` | técnico — reordenação de imports + warnings | +30 / −14 |
| `.gitignore` | técnico — adições (`.idea/`, `*.iml`, sistema operacional, etc.) | +12 |

### 2.2 Diff confirmado da subseção 6.5 ("Descrição de software")

**Saiu** (estava no template antigo, está no nosso `main` hoje):
- Diagrama BPMN
- Backlog funcional + backlog não-funcional como itens separados
- "Diagrama de casos de uso" como item explícito da seção
- "Diagrama de estados" como item explícito
- "Análise de dados (variáveis numéricas/gráficas)"
- "Protótipo funcional navegável"
- Roteiro de testes com campos `Tipo` (unitário/integrado/sistema) e `Reparo` + `Resultado pós-reparo`

**Entrou** (versão nova do Hilmer):
- **Diagrama de Atividades UML** (substitui BPMN) — exigência explícita de swimlanes, atores, atividades, insumos, decisões, paralelismo.
- **Backlog do Produto** unificado:
  - RF descritos via **História de Usuário (Eu-Como-Para)**.
  - RNF descritos objetivamente (com exemplo: "página carregar em até 5s em 4G").
  - **MoSCoW pra TUDO** (RF e RNF — antes era só RF).
  - **Protótipos de interface gráfica em alta fidelidade.**
  - **Todas HUs documentadas no GitHub com Eu-Como-Para + critérios de aceitação + protótipos.**
- **Arquitetura modelo 4+1 do Processo Unificado (UP)**:
  - Visão lógica · processos · implementação · implantação · dados.
  - Substitui a "visão de casos de uso" do modelo antigo.
  - Mantém: padrão arquitetural (MVC etc.), linguagens, frameworks, BD, persistência (MER+DER se relacional, ou diagrama de estrutura de documentos se NoSQL).
- **Roteiro de testes funcionais simplificado**: código, nome, objetivo, pré-condições (quando aplicável), procedimento, resultado esperado.

### 2.3 Implicações pro nosso projeto
- **Software vai precisar produzir:** 1 diagrama de atividades UML, ≥1 protótipo de interface alta-fidelidade do dashboard, documentação de todas as HUs no GitHub (issues ou docs/), descrição da arquitetura nos 5 pontos do 4+1 UP.
- **Nada do que sumiu (BPMN, casos de uso, estados, navegável) precisa entrar.** Não fazer trabalho desnecessário.
- A **EAP** (seção 6.1) não foi tocada no commit — formato segue o que Hilmer definiu na aula (ver § 4).

---

## 3 — Escopo PC1 (até item 6 do template — entrega 11/mai)

PC1 = entregar capítulos **3, 4, 5 e 6** do `relatorio.tex`. Capítulos 7–10 (orçamento, cronograma, resultados, lições) ficam pra PC2/IPP/APT.

### 3.1 Cap 3 — Introdução
**Pede o template:** revisão bibliográfica curta (≤ 2 páginas), pesquisas similares (Journals/Teses), legislações relevantes, indicadores de mercado se cabíveis, **1 parágrafo final de justificativa**.

| Campo | Valor |
|---|---|
| Dono primário | Giovanni (líder) |
| Contribuintes | 1 referência por sub-equipe (cada gerente sugere 1–2 papers/produtos similares na sua área) |
| Status | ⬜ vazio |
| Bloqueio | nenhum — pode começar a qualquer momento |

### 3.2 Cap 4 — Termo de Abertura do Projeto
**Pede o template:** dados (nome, código, gerente, patrocinador), objetivos **SMART**, mercado-alvo, requisitos do produto, justificativa, ≤ 10 indicadores.

| Campo | Valor |
|---|---|
| Dono primário | Giovanni (líder) |
| Status | ⬜ vazio |
| Dependência | nenhum bloqueio |

### 3.3 Cap 5 — Equipe de Trabalho
**Pede o template:** tabela com nome / matrícula / curso / telefone / email / atribuições. Tabela de avaliação só preenche no APT.

| Campo | Valor |
|---|---|
| Dono primário | Giovanni (consolida) |
| Contribuintes | cada gerente envia a lista da sua sub-equipe |
| Status | 🔵 parcial — gerentes definidos, membros HW/EN/ES incompletos |
| Pendência | ver § 8 — perguntas pra reunião |

### 3.4 Cap 6 — Projeto Conceitual do Produto · 5 seções

#### 6.1 Características gerais + EAP
**Pede o template:** descrição geral do produto, itens teóricos a aprofundar, **Estrutura Analítica do Projeto (EAP) apresentada e explicada** (em landscape se necessário pra legibilidade).

| Campo | Valor |
|---|---|
| Dono | Giovanni |
| Status | 🔵 rascunho deste roadmap |
| Pendência | confirmar formato com Hilmer (ver § 4) |

#### 6.2 Estrutura
**Pede o template:** desenho CAD, dimensões / cotas / aresta, materiais, decisões de projeto.

| Campo | Valor |
|---|---|
| Dono primário | Georgia (gerente Estrutura) |
| Contribuintes | sub-equipe Estrutura + Larissa (autora do labirinto) |
| Status | 🔵 em andamento (CAD planejado pra aula 29/04) |
| Entregas internas | rascunho 2D do rato · CAD base · CAD com pockets · CAD do labirinto · arquivo partdesign · texto sobre materiais |

#### 6.3 Descrição de hardware
**Pede o template:** diagrama de blocos + esquemático elétrico (KiCad recomendado — `protoboard NÃO é esquemático adequado`), pinagem dos componentes, justificativa das escolhas.

| Campo | Valor |
|---|---|
| Dono primário | Samuel (gerente HW/Eletrônica) |
| Contribuintes | João Rolim (esquemático), Aline, Pedro H., Guilherme (planilha) |
| Status | 🔵 lista de componentes fechada · esquemático em andamento (João Rolim) · orçamento misto loja+ML (Samuel) |
| Componentes confirmados (Samuel 25/04) | 1× ESP32 · 1× HC-SR04 (3× comprados pra reserva) · 1× MPU-6050 · 2× VL53L0X · 1× TB6612FNG · 2× N20 com encoder |
| Decisão técnica | resistor 1 kΩ entre HC-SR04 (5V) e ESP32 (3.3V) — datasheet HC-SR04 |

#### 6.4 Análise de consumo energético
**Pede o template:** identificação dos subsistemas elétricos (V, mA, t por componente) · cálculo $E = V \cdot I \cdot t$ por componente · estimativa total · escolha de fonte (com margem de segurança justificada) · circuito de alimentação (regulador, proteção, isolamento) · monitoramento via SW · validação real vs. teórico.

| Campo | Valor |
|---|---|
| Dono primário | Guilherme Quirino (gerente Energia) |
| **⚠️ Risco** | canal WhatsApp da Energia silencioso há 7+ dias (sem mensagens registradas na varredura 22–29/04). Sub-equipe sem outros nomes mapeados. **Se a Energia não engatar até quinta 1/mai, a 6.4 fica vazia e cascateia em 6.3 (regulador define o resto da placa).** |
| Decisão crítica pendente | **1S (3.7V) vs 2S (7.4V)** — define bateria, regulador, BMS |
| Mitigação | Giovanni faz 1:1 com Guilherme Quirino até quinta · se a sub-equipe não engatar, líder geral ajuda a estruturar a tabela de consumo (dados elétricos vêm da Energia) |

#### 6.5 Descrição de software · **versão Hilmer 22/04**
**Pede o template (pós-merge do PR §1):**
1. **Diagrama de Atividades UML** — fluxo do robô (varrer labirinto → flood fill → gravar → publicar). Swimlanes por componente (sensores · controle · telemetria). Ferramentas: draw.io ou PlantUML.
2. **Backlog do Produto** — RF como HU (Eu-Como-Para) + RNF objetivos + MoSCoW + protótipos alta-fidelidade do dashboard. **HUs documentadas no GitHub** (issues com label `user-story` ou docs em `docs/backlog/`).
3. **Arquitetura modelo 4+1 UP**: lógica (componentes do firmware + back + front) · processos (tasks dual-core no ESP32) · implementação (camadas em diretórios) · implantação (ESP32 + servidor + cliente) · dados (modelo de telemetria). Padrão (MVC?), linguagens, frameworks, BD.
4. **Roteiro de testes funcionais** — código, nome, objetivo, pré-cond (se aplicável), procedimento, resultado esperado. Conforme template Hilmer.

| Campo | Valor |
|---|---|
| Dono primário | Cairo (gerente SW) |
| Contribuintes | Gustavo Feitosa (arquitetura), Larissa, Giovanni, possível 4º membro `(a confirmar)`, Pedro Luciano de Azevedo (DevOps/Pages) |
| Status | 🔵 proposta de stack rascunhada por Gustavo (25/04) — **stack final ratificada hoje na reunião** (ver § 5) |

---

## 4 — EAP — formato Hilmer (a definir hoje na reunião)

### 4.1 Evidência disponível

| Fonte | Conteúdo | Confiança |
|---|---|---|
| Cairo no chat (27/04 ~20:29) | "conversamos com o Hilmer e ele passou esse jeito de fazer o EAP, sem separar as áreas, diferente do que a gente tinha visto antes" | 🟡 segunda-mão — a frase só elimina o formato por sub-equipe, não estabelece o que substitui |
| Giovanni no chat (29/04 13:03) | "to fazendo um rascunho da EAP pra gente, no final da aula de segunda conversamos com o hilmer tambem, ele deu umas ideias diferentes" | 🟡 reforça que houve conversa, não detalha formato |
| Material oficial | `aprender3.unb.br/.../projeto_conceitual_e_EAP.pdf` (Moodle UnB) | 🟡 **a baixar e anexar ao repo** |

### 4.2 Hipóteses (escolher antes de aplicar — perguntar ao Hilmer hoje)

**Hipótese A — EAP por fases do projeto (típica PMBOK):**

```
Micromouse Grupo 02
├── 1. Mobilização e definição
│   ├── 1.1 Termo de abertura
│   ├── 1.2 Equipe e papéis
│   └── 1.3 Plano de comunicação
├── 2. Projeto conceitual
│   ├── 2.1 Estrutura (CAD + materiais)
│   ├── 2.2 Hardware (esquemático + componentes)
│   ├── 2.3 Energia (consumo + fonte)
│   └── 2.4 Software (UML + backlog + arquitetura)
├── 3. Implementação
│   ├── 3.1 Chassis fabricado
│   ├── 3.2 Placa montada (protoboard → PCB)
│   ├── 3.3 Firmware (drivers + flood fill + telemetria)
│   └── 3.4 Dashboard
├── 4. Validação e integração
│   ├── 4.1 Testes unitários e integração
│   ├── 4.2 Testes em pista 4×4
│   └── 4.3 Testes em pista 8×8
└── 5. Comunicação e fechamento
    ├── 5.1 Relatórios PC1, PC2, IPP, APT
    ├── 5.2 Apresentações
    └── 5.3 Competição APT
```

**Hipótese B — EAP por componente do produto:**

```
Micromouse Grupo 02
├── 1. Robô (produto físico)
│   ├── 1.1 Estrutura mecânica
│   ├── 1.2 Eletrônica
│   ├── 1.3 Sistema de energia
│   └── 1.4 Software embarcado
├── 2. Pista de teste (labirinto Lego)
│   ├── 2.1 Módulo 4×4
│   └── 2.2 Módulo 8×8
├── 3. Sistema de telemetria e dashboard
│   ├── 3.1 Backend
│   └── 3.2 Frontend
├── 4. Documentação
│   ├── 4.1 Relatórios
│   ├── 4.2 GitPages
│   └── 4.3 Apresentações
└── 5. Gestão
    ├── 5.1 Reuniões e atas
    ├── 5.2 Cronograma
    └── 5.3 Orçamento
```

### 4.3 Recomendação

- **Antes da reunião de hoje:** Giovanni pergunta direto ao Hilmer ou olha o PDF do Aprender3. Sem essa confirmação, não imprimir EAP no relatório.
- **Hipótese A é palpite inicial** (mais alinhada com "sem separar áreas" do Cairo, porque B ainda mantém divisão por área nos sub-nós do "Robô"). Mas ambas são viáveis — confirmar com Hilmer antes de usar qualquer uma.
- **Decisão durável:** depois que Hilmer confirmar, atualizar este § 4 e anexar a EAP final como figura no relatório (provavelmente em `\begin{landscape}`).

---

## 5 — Stack Software ratificada · proposta de divisão (pra reunião hoje)

### 5.1 Decisão arquitetural (Giovanni 29/04)

| Camada | Linguagem | Motivo |
|---|---|---|
| **Firmware ESP32** | **C++** | classes facilitam padronização; C++ compila código C, então drivers da Eletrônica em C entram limpo |
| **Drivers de baixo nível** (motor, sensor, IMU) | **C** | feitos pela sub-equipe Eletrônica (Samuel + Aline + Pedro H. + João Rolim + Guilherme) `(a confirmar atribuição na reunião)` |
| **Backend** | **Python** (FastAPI) | proposta original do Gustavo (25/04) — leve, rápido pra MVP |
| **Banco** | **PostgreSQL** | relacional, suporta tempo real bem |
| **Frontend / Dashboard** | **React** | proposta Gustavo |

**Modelo de delegação SW ↔ Eletrônica:** Software desenha **diagrama de classes** especificando as interfaces que precisa (assinatura de funções). Eletrônica implementa os drivers em C **cumprindo essas interfaces**. C++ instancia os objetos e usa.

### 5.2 Exemplo de interface (rascunho — Cairo refina)

```cpp
// Software define a interface (header em src/firmware/include/)
class Motor {
public:
    // direcao: -1 (ré), 0 (parado), 1 (frente)
    // pwm: 0.0 a 1.0
    void run(int direcao, float pwm);
    void stop();
    long encoder_count();
};

class DistanceSensor {
public:
    enum Position { FRONT, LEFT, RIGHT };
    float read_cm();          // distância em cm
    bool wall_detected();     // booleano com threshold pré-calibrado
};

class IMU {
public:
    float yaw_degrees();      // ângulo Z em graus
    void reset_yaw();
};
```

Eletrônica implementa em C as funções `motor_run(dir, pwm)`, `sensor_read_cm(pos)` etc., e o C++ envolve em `extern "C"` no header.

> ⚠️ Esse é um esqueleto pra discussão, não recomendação de boas práticas embedded — Cairo + Gustavo ratificam ou ajustam (separar header/impl, uso de namespaces, gestão de erro, etc.).

### 5.3 Proposta de divisão das 4 camadas SW (pra Giovanni distribuir hoje)

> Time de Software tem 4 confirmados: **Cairo, Gustavo, Larissa, Giovanni**. Possível **5º membro** `(a confirmar)` — se entrar, redistribui donos de camada.

| Camada | Responsabilidade | Sugestão de dono | Por quê |
|---|---|---|---|
| **Algoritmo (flood fill)** | implementar flood fill, validar no mock simulator MMS, portar pro firmware | **Giovanni** | já tem MMS rodando localmente + mock_simulator.py (`~/Documents/unb/pi1/`) |
| **Firmware integrado** (RTOS, dual-core, integração com drivers C) | montar tasks core0/core1, integrar drivers da Eletrônica, telemetria via Wi-Fi | **Cairo** | gerente + tech lead, conhece a stack e fala com a Eletrônica |
| **Backend (FastAPI + PostgreSQL)** | API que recebe telemetria, persiste, expõe pro front | **Gustavo Feitosa** | autor da proposta de arquitetura, conhece o stack |
| **Frontend (React + dashboard)** | visualização do labirinto, posição em tempo real, métricas | **Larissa** ou 4º membro `(a confirmar)` | Larissa tem perfil dev — confirmar interesse |

**Suporte transversal:**
- **Pedro Luciano de Azevedo** — DevOps / GitPages / MkDocs / CI (já está fazendo). Não precisa virar dono de camada.

### 5.4 O que cada um precisa entregar até PC1 (11/mai)

- **Giovanni:** spec do flood fill em pseudo-código (Doc atual: `docs.google.com/document/d/12CVu5VQPweENysIuWc6OSgXULO73K0a5PBQWVMMucO8`). Vai pro relatório como parte da arquitetura 4+1 UP — visão lógica.
- **Cairo:** diagrama de atividades UML (fluxo do robô) + arquitetura 4+1 UP escrita.
- **Gustavo:** diagrama de classes (interfaces que a Eletrônica vai implementar) + esboço do schema do PostgreSQL (visão de dados).
- **Larissa / 4º** `(a confirmar)`: wireframes alta-fidelidade do dashboard (Figma ou similar).
- **Todos:** abrir ≥3 issues `user-story` no GitHub no formato Eu-Como-Para + critérios de aceitação + link do protótipo.

---

## 6 — Roadmap por release

> **Releases acadêmicas** (pesos NF): R1 = PC1 (0,15) · R2 = PC2 (0,15) · R3 = IPP (0,15) · R4 = APT (0,25) · AI contínua (0,30). _(fórmula vinda do plano de ensino PI1 2026.1 — confirmar no `.tex` oficial)_
> **Aprovação:** AI ≥ 5, NF ≥ 5, freq ≥ 75%.

### R1 — PC1 (11/mai) · "documentação completa, projeto definido"

| Tese | Caps 3+4+5+6 do relatório completos. Robô não precisa rodar. EAP no formato Hilmer. **Simulador MMS NÃO entra** (Giovanni confirmou 29/04). |
|---|---|

**Entregas duras:**
- 📄 Relatório PC1 (caps 3–6) compilado em PDF.
- 📐 EAP no formato Hilmer (figura na seção 6.1).
- 📊 CAD do chassis + dimensões + materiais (seção 6.2).
- ⚡ Diagrama de blocos + esquemático KiCad (seção 6.3).
- 🔋 Análise de consumo + escolha de bateria + 1S/2S decidido (seção 6.4).
- 🤖 Diagrama de Atividades UML + Backlog (HU+MoSCoW+protótipos) + Arquitetura 4+1 UP + Roteiro de testes (seção 6.5).
- 🌐 GitHub repo populado (HUs documentadas como issues `user-story`).
- 🎤 Apresentação PC1 (slides existem em `Micromouse_PI1_Grupo02.pptx` — revisar).

### R2 — PC2 (9/jun) · "protótipo parcial"

**Tese:** componentes chegaram, integrações iniciais funcionando, software validado em sim.

- 🤖 Flood fill rodando no **simulator MMS** (mock_simulator.py + 3 topologias).
- ⚙️ Firmware ESP32 com drivers básicos (motor, ToF, IMU) — bench test.
- 🔌 Protoboard com sensores + ponte H + ESP32 ligados; motor girando.
- 🔋 Bateria + regulador testados em bancada.
- 🏗️ Chassis CAD finalizado, primeira peça impressa/cortada.
- 🟢 Pista 4×4 montada e usável.
- 📄 Relatório PC2 (delta sobre PC1 + resultados parciais).
- 📈 Capítulos 7 (Orçamento) e 8 (Cronograma) preenchidos.

### R3 — IPP (29/jun) · "robô integrado"

**Tese:** robô anda, mapeia, telemetria sai pelo dashboard.

- 🤖 Flood fill rodando no firmware (não só sim).
- 📡 Telemetria MQTT/HTTP → dashboard mostrando posição/decisões em tempo real.
- 🏎️ Robô percorre 1 labirinto 4×4 sem intervenção.
- 🔋 Autonomia mínima validada.
- 🏗️ Chassis final v1 com componentes montados.
- 📄 Mini-relatório IPP / vídeo demo (90s).
- 📈 Capítulo 9 (Resultados) iniciado.

### R4 — APT (julho) · "competição"

**Tese:** robô compete em 3 labirintos, equipe defende projeto.

- 🏆 Robô em 3 labirintos da competição (8×8 quando aplicável).
- 📈 Otimização de tempo / passos / estratégia de reset.
- 📄 Relatório final consolidado (caps 3–10 completos).
- 🎤 Apresentação APT — defesa do projeto.
- 📦 Documentação técnica entregue.

---

## 7 — User Stories — backlog estruturado

> IDs estáveis por área: `SW`, `HW`, `EN`, `ES`, `INT`, `DOC`, `LID`. Toda US referenciável pelo ID nos chats.
> **Esforço:** P (≤1d) · M (1–3d) · G (3–7d) · GG (>7d). **Estimativas são chutes não-calibrados** — rever na retrospectiva da R1.
> **Status:** ⬜ a fazer · 🔵 em andamento · ✅ feito · ⚠️ bloqueado · 🟡 a confirmar.

### 7.1 Documentação (DOC)

| ID | Título | Dono | Release | Esf | Status | Dep |
|---|---|---|---|---|---|---|
| DOC-01 | Mergear PR `sync/template-hilmer-22-04` no `main` | Cairo / Giovanni | R1 | P | 🔵 PR pronto | — |
| DOC-02 | Cap 3 Introdução (≤ 2 pgs + revisão biblio) | Giovanni + 1 ref por sub-eq | R1 | M | ⬜ | DOC-01 |
| DOC-03 | Cap 4 Termo de Abertura (SMART + req + indicadores) | Giovanni | R1 | M | ⬜ | DOC-01 |
| DOC-04 | Cap 5 Equipe (tabela completa) | Giovanni (consolida) | R1 | P | 🔵 parcial | gerentes mandam nomes |
| DOC-05 | Seção 6.1 — Características gerais + EAP figura | Giovanni | R1 | M | 🔵 rascunho | § 4 confirmado |
| DOC-06 | Seção 6.2 — Estrutura (CAD + materiais) | Georgia | R1 | M | 🔵 | ES-01..03 |
| DOC-07 | Seção 6.3 — Hardware (blocos + esquemático) | Samuel | R1 | M | 🔵 | HW-02 |
| DOC-08 | Seção 6.4 — Energia (consumo + fonte) | Guilherme Quirino | R1 | M | ⚠️ | EN-01..02 |
| DOC-09 | Seção 6.5 — Software (UML + backlog + 4+1 + testes) | Cairo + SW | R1 | G | ⬜ | SW-04..07 |
| DOC-10 | Compilar PDF PC1 + revisar | Giovanni | R1 (até 10/mai) | P | ⬜ | DOC-02..09 |
| DOC-11 | Apresentação PC1 — revisar slides existentes | Giovanni | R1 | P | 🔵 | — |
| DOC-12 | Cap 7 Orçamento (planejado vs realizado) | Samuel | R2 | M | ⬜ | HW-01 |
| DOC-13 | Cap 8 Cronograma (planejado vs realizado) | Giovanni | R2 | M | ⬜ | este doc |
| DOC-14 | Cap 9 Resultados (testes V&V) | todos | R3 | G | ⬜ | INT-01 |
| DOC-15 | Cap 10 Lições aprendidas + SWOT | Giovanni + gerentes | R4 | M | ⬜ | — |
| DOC-16 | GitPages atualizado com estrutura do projeto | Pedro L. de Azevedo | R1→R4 | P (recorrente) | 🔵 ativo | — |

### 7.2 Software (SW)

| ID | Título | Dono | Release | Esf | Status | Dep |
|---|---|---|---|---|---|---|
| SW-01 | Ratificar stack na reunião 29/04 (este doc) | Cairo + Giovanni | R1 | P | 🔵 | — |
| SW-02 | Definir 4 donos das camadas (algoritmo / firmware / back / front) | Cairo | R1 | P | 🔵 (proposta no § 5) | — |
| SW-03 | Reenviar convites GitHub que expiraram | Cairo | R1 | P | ⬜ | — |
| SW-04 | Diagrama de Atividades UML (fluxo robô) | Cairo | R1 | M | ⬜ | DOC-01 |
| SW-05 | Diagrama de classes (interfaces dos drivers) | Gustavo | R1 | M | ⬜ | SW-01 |
| SW-06 | Backlog: ≥10 HUs no GitHub (Eu-Como-Para + criterio + protótipo) | Cairo distribui | R1 | G | ⬜ | SW-05 |
| SW-07 | Wireframes alta-fidelidade do dashboard | Larissa ou 4º | R1 | M | ⬜ | SW-02 |
| SW-08 | Arquitetura 4+1 UP escrita (5 visões) | Cairo + Gustavo | R1 | M | ⬜ | SW-04, SW-05 |
| SW-09 | Roteiro de testes funcionais (≥6 casos) | Cairo | R1 | M | ⬜ | SW-06 |
| SW-10 | Spec flood fill em pseudo-código | Giovanni | R1 | M | 🔵 (Google Doc) | — |
| SW-11 | Implementar flood fill v1 no mock | Giovanni | R2 | G | ⬜ | SW-10 |
| SW-12 | Validar flood fill em ≥3 topologias do mock | Giovanni | R2 | M | ⬜ | SW-11 |
| SW-13 | Driver wrappers C++ pros C dos ToF/Motor/IMU | Cairo | R2 | M | ⬜ | HW-03, drivers C |
| SW-14 | Backend FastAPI + schema PostgreSQL | Gustavo | R2→R3 | G | ⬜ | SW-05 |
| SW-15 | Dashboard React (visualização labirinto + métricas) | Larissa / 4º | R3 | G | ⬜ | SW-14 |
| SW-16 | Telemetria MQTT/HTTP do firmware | Cairo | R3 | M | ⬜ | SW-13 |
| SW-17 | CI build (GitHub Actions) | Pedro L. de Azevedo | R2 | M | ⬜ | DOC-01 |
| SW-18 | Portar flood fill validado pro firmware | Giovanni | R3 | M | ⬜ | SW-12, SW-13 |
| SW-19 | Tuning final flood fill (heurísticas APT) | Giovanni | R4 | G | ⬜ | SW-18, INT-02 |
| SW-20 | Estratégia de reset (decidir quando voltar à origem) | Giovanni | R4 | M | ⬜ | SW-19 |

### 7.3 Hardware/Eletrônica (HW)

| ID | Título | Dono | Release | Esf | Status | Dep |
|---|---|---|---|---|---|---|
| HW-01 | Lista de compra finalizada em planilha (link, preço, qty) | Samuel | R1 (até 1/mai) | P | 🔵 orçando misto | EN-01 |
| HW-02 | Diagrama de blocos (ESP32⇄drivers⇄motores; ESP32⇄I²C⇄ToF/IMU; bat⇄reg⇄ESP32) | Samuel | R1 | M | ⬜ | EN-01 |
| HW-03 | Esquemático elétrico no KiCad (não protoboard) | João Rolim | R1 | G | 🔵 iniciado | HW-01 |
| HW-04 | Drivers em C: motor (TB6612), ToF (VL53L0X + HC-SR04), IMU (MPU-6050) | sub-eq Eletrônica (Samuel atribui) | R2 | G | ⬜ | SW-05 |
| HW-05 | Protoboard funcional — primeiro `motor_run()` | Samuel | R2 | G | ⬜ | HW-03 |
| HW-06 | Bring-up IMU + calibração inicial | Samuel + Gustavo | R2 | M | ⬜ | HW-05 |
| HW-07 | Decisão protoboard vs PCB pra R3 | Samuel | R2 | P | ⬜ | HW-05 |
| HW-08 | PCB v1 (se decisão = PCB) | Samuel | R3 | GG | ⬜ | HW-04, HW-07 |
| HW-09 | Calibração final de sensores em pista real | Samuel + SW | R3 | M | ⬜ | HW-05, ES-04 |
| HW-10 | HW definitivo montado no chassis | Samuel + Estrutura | R3 | M | ⬜ | HW-08 ou HW-05, ES-06 |

### 7.4 Energia (EN)

| ID | Título | Dono | Release | Esf | Status | Dep |
|---|---|---|---|---|---|---|
| EN-01 | **Decisão 1S vs 2S** | Guilherme Quirino | R1 (até 1/mai) | P | ⚠️ bloqueia HW | — |
| EN-02 | Análise de consumo (V·I·t por componente) | Guilherme Quirino | R1 | M | ⬜ | — |
| EN-03 | Especificar bateria LiPo (capacidade, conector) | Guilherme Quirino | R1→R2 | M | ⬜ | EN-01 |
| EN-04 | Selecionar regulador 5V/3.3V (linear vs buck) | Guilherme Quirino + Samuel | R2 | M | ⬜ | EN-01 |
| EN-05 | BMS / proteção bateria | Guilherme Quirino | R2 | M | ⬜ | EN-03 |
| EN-06 | Teste de autonomia em bancada | Guilherme Quirino | R2→R3 | M | ⬜ | HW-05 |
| EN-07 | Teste de autonomia em pista | Guilherme Quirino | R3 | M | ⬜ | INT-01 |
| EN-08 | Reativar canal de comunicação Energia | Guilherme Quirino | R1 (até 1/mai) | P | ⚠️ | — |
| EN-09 | Listar membros da sub-equipe Energia | Guilherme Quirino | R1 | P | ⬜ | — |

### 7.5 Estrutura (ES)

| ID | Título | Dono | Release | Esf | Status | Dep |
|---|---|---|---|---|---|---|
| ES-01 | EAP de Estrutura (interna do grupo Estrutura) | Georgia | R1 (29/04) | P | 🔵 hoje | — |
| ES-02 | Rascunho 2D do rato (vista superior + lateral, dimensões) | Georgia | R1 | P | ⬜ | — |
| ES-03 | CAD base do rato (CATIA V5) | Georgia | R1→R2 | G | 🔵 (PC na aula 29/04) | ES-02 |
| ES-04 | CAD pocket (espaço pros componentes) | Georgia | R2 | M | ⬜ | ES-03, HW-02 |
| ES-05 | CAD do labirinto (modular Lego) | Georgia | R1 | M | 🔵 (aula 29/04) | — |
| ES-06 | Plano de fabricação (impressão 3D / corte / madeira) | Georgia | R2 | M | ⬜ | ES-03 |
| ES-07 | Fabricar chassis v1 | Georgia | R2 | G | ⬜ | ES-06 |
| ES-08 | Fixação dos componentes no chassis | Georgia + Samuel | R3 | M | ⬜ | ES-07, HW-10 |
| ES-09 | Pista 4×4 Lego — manutenção e ajustes | Larissa + sub-eq Estrutura | R1→R4 | P (recorrente) | ✅ entregue (Larissa+pai marceneiro) | — |
| ES-10 | Pista 8×8 — segundo módulo | `(a definir)` | R3→R4 | G | ⬜ | ES-09 |
| ES-11 | Texto sobre escolha de materiais | Georgia | R1 | M | ⬜ | ES-03 |
| ES-12 | Listar membros da sub-equipe Estrutura | Georgia | R1 | P | ⬜ | — |

### 7.6 Integração e Liderança (INT/LID)

| ID | Título | Dono | Release | Esf | Status | Dep |
|---|---|---|---|---|---|---|
| INT-01 | Bring-up integrado: robô anda 1m reto sob firmware | Giovanni + Cairo + Samuel | R3 | G | ⬜ | SW-18, HW-10, EN-04 |
| INT-02 | Primeiro labirinto 4×4 percorrido autonomamente | todos | R3 | G | ⬜ | INT-01, ES-09 |
| INT-03 | Tuning velocidade + estabilidade | todos | R4 | G | ⬜ | INT-02 |
| INT-04 | 3 labirintos da competição | todos | R4 | G | ⬜ | INT-03 |
| LID-01 | Reunião semanal segunda 16h–18h + ata | Giovanni | recorrente | P | 🔵 | — |
| LID-02 | Atualizar este roadmap toda segunda antes da reunião | Giovanni | recorrente | P | 🔵 | — |
| LID-03 | Confirmar formato EAP com Hilmer (Hipótese A vs B) | Giovanni | R1 (até 4/mai) | P | 🔵 | — |
| LID-04 | RACI atualizado por entregável | Giovanni | R1 | P | 🔵 (este doc) | DOC-04 |

---

## 8 — Mapa por sub-equipe (operacional, **interno do grupo**)

> Esta seção é gestão. **Não copiar pro relatório.** Tem julgamento sobre membros que serve pra coordenação interna, não pra entrega acadêmica.

### 8.1 Software · Cairo

**Membros confirmados:** Cairo, Gustavo Feitosa, Larissa, Giovanni · possível **5º** `(a confirmar)` · Pedro L. de Azevedo (DevOps transversal).

**3 perguntas pra fechar até segunda 4/mai:**
1. Tem 4º dev além de Cairo/Gustavo/Larissa/Giovanni?
2. Quem assume frontend (Larissa ou 4º)? — proposta no § 5.
3. Stack final ratificada — ainda há objeção a C++ no firmware ou Python no backend?

### 8.2 Hardware/Eletrônica · Samuel

**Membros confirmados:** Samuel (gerente), João Rolim, Aline, Pedro H., Guilherme (LID 110642786779265 — **≠ Guilherme Quirino** da Energia).

**3 perguntas pra fechar até segunda 4/mai:**
1. Há mais alguém na sub-equipe além desses 5?
2. Decisão final — comprar 1 ou 3 HC-SR04? (esquemático usa 1; comprar 3 pra reserva está OK?)
3. Quem implementa cada driver C (motor, ToF, IMU)? Cairo precisa pra dimensionar timeline da SW-13.

### 8.3 Energia · Guilherme Quirino

**Membros confirmados:** Guilherme Quirino (gerente). Resto desconhecido.

**Antes de pergunta técnica — diagnóstico:**
- O canal de WhatsApp da Energia está silencioso há 7+ dias (sem mensagens registradas na varredura 22–29/04).
- 1ª pergunta na reunião de hoje: o canal monitorado é o canal real? Tem outro canal? O time engatou ou empacou?

**3 perguntas técnicas (depois do diagnóstico):**
1. **1S (3.7V) ou 2S (7.4V)?** Decisão dura. Define bateria, regulador, BMS.
2. Análise de consumo grosseira: ESP32 (~150 mA pico) + 2× N20 (~500 mA pico cada em stall) + 2× VL53L0X (~20 mA cada) + 1× MPU-6050 (~3 mA) + 1× HC-SR04 (~15 mA). Total estimado: ?
3. Listar nomes da sub-equipe.

### 8.4 Estrutura · Georgia

**Membros confirmados:** Georgia (gerente), Larissa (autora do labirinto). Mais alguém `(a confirmar)`.

**3 perguntas pra fechar até segunda 4/mai:**
1. Quem mais está na sub-equipe Estrutura?
2. CAD do labirinto na aula 29/04 — quem participa além da Georgia?
3. Fabricação chassis — impressão 3D (FCT/laboratório) ou corte de MDF?

---

## 9 — Gaps a fechar até segunda 4/mai

1. **Mergear PR `sync/template-hilmer-22-04`** — Cairo. Url: `github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana/pull/new/sync/template-hilmer-22-04`.
2. **Confirmar formato EAP com Hilmer** (Hipótese A vs B) — Giovanni. Pergunta direta na próxima oportunidade ou consultar PDF do Aprender3.
3. **Ratificar stack SW + 4 donos das camadas** — Cairo + Giovanni. Reunião de hoje.
4. **EN-01: 1S vs 2S** — Guilherme Quirino. Até 1/mai.
5. **EN-08: reativar canal Energia** — Guilherme Quirino. Até 1/mai.
6. **Membros nomeados em HW/EN/ES** — gerentes mandam pra Giovanni até 1/mai.
7. **HW-01: lista de compra fechada em planilha** — Samuel. Até 1/mai.
8. **DOC-04: tabela completa do cap 5** — Giovanni consolida quando 6 chegar.

---

## 10 — Backlog priorizado · próximas 2 semanas

### Esta semana (29/abr–4/mai)
1. DOC-01 — merge PR template (Cairo).
2. SW-01, SW-02 — stack ratificada + 4 donos (reunião hoje).
3. EN-01 — 1S vs 2S (Guilherme Quirino).
4. HW-01 — planilha compra (Samuel).
5. ES-01, ES-02, ES-05 — EAP Estrutura + rascunho rato + CAD labirinto (Georgia, aula 29/04).
6. SW-04, SW-05, SW-10 — diagramas SW + spec flood fill iniciados.
7. LID-03 — confirmar formato EAP com Hilmer (Giovanni).
8. EN-02 — análise consumo grosseira (Guilherme).

### Semana de 5/mai–11/mai (semana de entrega)
9. DOC-02..09 — preencher caps 3, 4, 5 e seções 6.1–6.5 do relatório.
10. DOC-10 — compilar PDF + revisar.
11. DOC-11 — revisar slides PC1.
12. Ensaio interno do grupo na sexta 9/mai (sugestão).

---

## 11 — Pontos abertos / a confirmar

- 🟡 **Formato exato da EAP do Hilmer** — Hipótese A (por fases) parece mais provável, mas não confirmado. Validar em LID-03.
- 🟡 **Composição completa de HW, EN e ES** — gerentes mandam até 1/mai.
- 🟡 **4º membro da Software** — existe? quem é?
- 🟡 **Larissa quer assumir o dashboard React?** — propor na reunião.
- 🟡 **Decisão protoboard vs PCB pra IPP** (HW-07) — depende de tempo + grana, decisão na PC2.
- 🟡 **Pista 8×8 (ES-10)** — quem constrói o segundo módulo? Larissa+pai de novo? Comunidade?
- 🟡 **Composição dos drivers C** — Samuel atribui na sub-equipe.

---

## 12 — Restrições do template (apêndice fixo)

- **Repo:** `<ano>.<semestre>_PI1_Grupo<n>_<professor>` = `2026.1_PI1_Grupo02_Juliana` (atual `2026_1_PI1_Grupo02_Juliana` com underscores — funcional, mas a convenção do template usa pontos; vale conferir com Juliana se precisa renomear).
- **Equipe GitHub:** `<repo>_Equipe`.
- **Tamanho:** arquivos > 5 MB ficam fora do repo (link em outra plataforma).
- **Estrutura de pastas:** `docs/` (relatório) · `hw/` (eletrônica — KiCad, simulações, datasheets, CSVs) · `mec/` (CAD, STL, desenhos técnicos) · `src/{firmware, backend, frontend}` (código).
- **Hierarquia de pastas NÃO reflete sub-equipe** — todos podem mexer em qualquer pasta.

> Nota sobre IA: este roadmap foi estruturado com apoio de uma ferramenta de IA do Giovanni — usada para consolidar o conteúdo do template, varredura dos canais do projeto e decisões da liderança em um único mapa-mestre. Conteúdo técnico (HUs, descrições de hardware, cálculos elétricos, código) é produzido pelo time. O README do template menciona "não é permitida a utilização de IA" no contexto de estudos técnicos da pasta `mec/`; a interpretação aplicada aqui é que isso cobre estudos técnicos avaliativos, não a organização interna do projeto. **Confirmar com a prof. Juliana se a leitura é correta** — caso contrário, este doc fica como artefato pessoal da liderança e o conteúdo migra pra documento manual.

---

## 13 — Como manter este doc vivo

- **Toda segunda 14:00–16:00:** Giovanni revisa status das USs antes da reunião 16h.
- **Cada US fechada vira nota de ata** em `docs/meetings/YYYY-MM-DD.md` no repo do grupo.
- **Mudanças de escopo ou dono:** atualizar aqui + avisar no chat geral.
- **Nova US descoberta:** adicionar com ID sequencial (`SW-21`, `HW-11`, etc.). Não reusar ID retirado.
- **Trocou release de uma US?** Marcar motivo numa coluna "obs" temporária; conversar em reunião.

---

## 14 — Histórico de versões

- **v1 — 2026-04-29** · primeira versão consolidada · cruza template Hilmer (commits 22/04) + varredura dos canais do projeto 22–29/04 + decisões da liderança 29/04 (stack C++ firmware + C drivers, simulador fora da PC1, formato EAP a confirmar com Hilmer).
- v0 (`_archive/eap-v1-pre-roadmap-2026-04-29.md`) · esboço inicial só com EAP por sub-equipe — substituído.

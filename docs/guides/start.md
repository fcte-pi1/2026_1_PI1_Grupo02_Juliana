# Como começar

## Clonar o repositório

```bash
git clone https://github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana.git
cd 2026_1_PI1_Grupo02_Juliana
```

## Configurar o ambiente

### Opção A: Virtualenv (Python)

```bash
# Criar virtual environment
python -m venv .venv

# Ativar (Windows)
.venv\Scripts\activate

# Ativar (Linux/Mac)
source .venv/bin/activate

# Instalar dependências
pip install -r requirements.txt
```

### Opção B: Conda

```bash
conda create -n pi1 python=3.11
conda activate pi1
pip install -r requirements.txt
```

## Rodar o MkDocs localmente

```bash
# Navegar até a pasta do projeto
cd 2026_1_PI1_Grupo02_Juliana

# Rodar servidor de desenvolvimento
mkdocs serve

# Site estará em: http://localhost:8000
```

---

## Estrutura do projeto

```
2026_1_PI1_Grupo02_Juliana/
│
├── .github/
│   ├── ISSUE_TEMPLATE/       (templates)
│   └── PULL_REQUEST_TEMPLATE/
│
├── docs/
│   ├── index.md              (Home)
│   ├── _config.yml           (Config Jekyll)
│   └── mkdocs.yml            (Config MkDocs)
│
├── src/
│   ├── backend/
│   ├── firmware/
│   └── frontend/
│
├── hw/                       (Hardware)
├── mec/                      (Mecânica)
│
├── mkdocs.yml               (Config MkDocs)
├── requirements.txt         (Dependências Python)
├── CONTRIBUTING.md
└── README.md
```

---

## Checklist inicial

- [ ] Git instalado e configurado
- [ ] Python 3.11+ instalado
- [ ] Repositório clonado localmente
- [ ] Virtual environment criado e ativado
- [ ] `pip install -r requirements.txt` executado
- [ ] `mkdocs serve` rodando em http://localhost:8000
- [ ] Site carregando corretamente

---

## Próximas ações

1. Leia o [guia de contribuição](contributing.md).
2. Consulte a [referência rápida](quick-reference.md) quando precisar.

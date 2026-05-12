# **1 - Instalando o VS Code e Git:** 

Os links abaixo levam diretamente para página de download.

- [Visual Studio Code](https://code.visualstudio.com/download)
- [Git](https://git-scm.com/)

Após as intalações, abra o VS Code e faça o login com a sua conta do Github.

![login1](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/v5xTCTT_y2nbho)

![login2](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/eHds4mx_e00r02)

Após isso, selecionar a opção de github e você será direcionado para uma aba no navegador para finalizar o login.


## **Clonar o repositório**

Dentro de uma pasta para a disciplina ou na Área de Trabalho, aperte o botão direito do mouse e clique em **"Open Git Bash here"**.

![GitBash](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image_wkbsjz)

Com o terminal aberto, copie a primeira linha abaixo e cole no terminal clicando com o botão direito do mouse e selecionando **"Paste"**.

```bash
git clone https://github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana.git
```

![terminal](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image1_iphsbs)

Após isso, cole a linha abaixo para entrar na pasta do projeto. 

```bash
cd 2026_1_PI1_Grupo02_Juliana
```
Dentro a pasta do projeto digite o comando abaixo para abrir o arquivo com o VS Code.

```bash
code .
```

![abrir vscode](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image2_ri9jqb)

Outra alternativa para abrir o projeto é clicando com o botão direito no arquivo clonado e escolhendo a opção **"Abrir com Code"**.

![abrirCode](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image3_qdhhwh)

Quando você abrir um projeto no VS Code, ele será reaberto automaticamente na próxima vez que você iniciar o programa. Dessa forma, só é necessário repetir a última estapa caso você esteja mexendo com diferentes projetos no VS Code.


## **Instale o Python**

Caso você não tenha python instalado na sua máquina, siga essa etapa.

- [Python](https://www.python.org/)

Coloque o mouse em cima de downloads e depois clique em **"Python 3.14.4"**.

![downloadpython](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image4_lv3hij)

Antes de instalar, marque as duas opções abaixo e depois clique em **"Install Now"**.

![oppy](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image6_bvbxf9)

## **Configurar o ambiente**

Com o projeto aberto no VS Code, aperte as teclas **Ctrl + '** para abrir o terminal. Copie, cole e execute no terminal as linhas a seguir na respectiva ordem.

### Criando venv usando o Python

Criar virtual environment

```bash
python -m venv .venv
```
Ativar (Windows)

```bash
.venv\Scripts\activate
```
Ativar (Linux/Mac)

```bash
source .venv/bin/activate
```
Instalar dependências

```bash
pip install -r requirements.txt
```

### Outra alternativa usando Conda

```bash
conda create -n pi1 python=3.11
conda activate pi1
pip install -r requirements.txt
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

---

## Próximas ações

1. Leia o [guia de contribuição](contributing.md).
2. Consulte a [referência rápida](quick-reference.md) quando precisar.

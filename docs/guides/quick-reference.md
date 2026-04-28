# Referência rápida

## Git - Comandos Essenciais

```bash
# Clonar
git clone https://github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana.git

# Atualizar
git fetch origin
git pull origin main

# Criar branch
git checkout -b feature/nome

# Ver status
git status

# Adicionar mudanças
git add .

# Commit
git commit -m "tipo: descrição"

# Push
git push origin feature/nome

# Ver branches
git branch

# Deletar branch local
git branch -d feature/nome
```

---

## MkDocs - Rodar localmente

```bash
# Ativar virtual environment (Windows)
.venv\Scripts\activate

# Ativar virtual environment (Linux/Mac)
source .venv/bin/activate

# Instalar/atualizar dependências
pip install -r requirements.txt

# Rodar servidor
mkdocs serve

# Acessar: http://localhost:8000
```

---

## Tipos de commit

| Tipo | Descrição | Exemplo |
|------|-----------|---------|
| `feat:` | Nova funcionalidade | `feat: adicionar dark mode` |
| `fix:` | Correção de bug | `fix: corrigir validação` |
| `docs:` | Documentação | `docs: atualizar README` |
| `style:` | Formatação/estilo | `style: corrigir indentação` |
| `refactor:` | Refatoração | `refactor: simplificar função` |
| `test:` | Testes | `test: adicionar testes unitários` |
| `chore:` | Manutenção | `chore: atualizar dependências` |

---

---

## Tipos de issue

| Tipo | Quando usar |
|------|-----------|
| `[BUG]` | Encontrou um erro |
| `[FEATURE]` | Quer sugerir algo novo |
| `[DOC]` | Documentação desatualizada |
| `[TASK]` | Tarefa ou item de trabalho |

---

## Workflow padrão (5 passos)

```
1. git checkout -b feature/nome
2. Editar, commit, push
3. Abrir PR no GitHub
4. Preencher template (automático)
5. Aguardar review e merge
```

---

## Configurar Git (primeira vez)

```bash
git config --global user.name "Seu Nome"
git config --global user.email "seu.email@example.com"
git config --global core.editor "code"
```

---

## Desfazer mudanças

```bash
# Desfazer último commit (manter mudanças)
git reset --soft HEAD~1

# Desfazer último commit (descartar)
git reset --hard HEAD~1

# Desfazer mudanças em arquivo específico
git checkout -- arquivo.txt

# Ver histórico
git log --oneline
```

---

## Sincronizar com upstream (forks)

```bash
# Adicionar upstream
git remote add upstream https://github.com/fcte-pi1/...

# Atualizar com upstream
git fetch upstream
git rebase upstream/main

# Push
git push origin main
```

---

## URLs importantes

- **Repositório**: https://github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana
- **GitHub Pages**: https://fcte-pi1.github.io/2026_1_PI1_Grupo02_Juliana
- **Issues**: Clique em "Issues" no GitHub
- **PRs**: Clique em "Pull Requests" no GitHub

---

## Atalhos do terminal

| Comando | O que faz |
|---------|----------|
| `cd` | Navegar pasta |
| `ls` | Listar arquivos |
| `mkdir` | Criar pasta |
| `rm` | Deletar arquivo |
| `clear` | Limpar tela |
| `pwd` | Mostrar pasta atual |

---

**Última atualização**: 28 de Abril de 2026

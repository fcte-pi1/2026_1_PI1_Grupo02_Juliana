# Contribuindo para o Projeto

Obrigado por se interessar em contribuir para este projeto! Este documento fornece diretrizes e instruções para contribuir de forma eficaz.

## Código de Conduta

Todos os colaboradores devem se comportar de forma respeitosa e profissional. A falta de respeito resultará em remoção do projeto.

## Como Começar

### Configuração do Ambiente

1. **Faça um fork do repositório** em sua conta do GitHub
2. **Clone o repositório forkado** localmente:
   ```bash
   git clone https://github.com/seu-usuario/2026_1_PI1_Grupo02_Juliana.git
   cd 2026_1_PI1_Grupo02_Juliana
   ```

3. **Adicione o repositório original como upstream**:
   ```bash
   git remote add upstream https://github.com/fcte-pi1/2026_1_PI1_Grupo02_Juliana.git
   ```

4. **Crie uma branch para sua contribuição**:
   ```bash
   git checkout -b feature/sua-feature
   ```

## Estrutura do Repositório

O projeto está organizado da seguinte forma:

```
2026_1_PI1_Grupo02_Juliana/
├── docs/           # Documentação e relatório
├── hw/             # Hardware (esquemáticos, CAD, etc)
├── mec/            # Componentes mecânicos
├── src/            # Código-fonte
│   ├── backend/
│   ├── firmware/
│   └── frontend/
└── README.md
```

## Fluxo de Trabalho

### 1. Abrindo uma Issue

Antes de começar a trabalhar, abra uma issue para:
- **Reportar bugs**: Use o template "Bug Report"
- **Sugerir funcionalidades**: Use o template "Feature Request"
- **Melhorias de documentação**: Use o template "Documentação"

### 2. Desenvolvendo

- **Commits descritivos**: Use mensagens de commit claras e concisas
  ```bash
  git commit -m "feat: adicionar validação de entrada"
  git commit -m "fix: corrigir bug no cálculo"
  git commit -m "docs: atualizar documentação de instalação"
  ```

- **Convenção de nomes de branches**:
  - `feature/descricao-da-feature` - nova funcionalidade
  - `fix/descricao-do-bug` - correção de bug
  - `docs/descricao-da-doc` - documentação
  - `refactor/descricao` - refatoração
  - `test/descricao` - testes

### 3. Submetendo um Pull Request

1. **Atualize sua branch com a branch principal**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Envie suas mudanças para seu fork**:
   ```bash
   git push origin sua-branch
   ```

3. **Abra um Pull Request** no repositório original com o template fornecido

4. **Preencha o template completamente**:
   - Descrição clara das mudanças
   - Tipo de mudança
   - Issues relacionadas
   - Testes realizados
   - Checklist completo

### 4. Revisão de Código

- Todos os PRs devem ser revisados por pelo menos um outro membro
- Responda aos comentários dos revisores prontamente
- Faça as alterações solicitadas em novos commits (não force push sem coordenação)
- Mantenha uma atitude construtiva e aberta a feedback

## Diretrizes de Código

### Geral

- Mantenha o código limpo e legível
- Comente código complexo
- Siga o estilo de código existente
- Não adicione arquivos desnecessários ou maiores que 5MB

### Documentação

- Atualize o README se adicionar novas funcionalidades
- Documente funções/métodos públicos
- Mantenha a documentação atualizada

### Testes

- Escreva testes para novas funcionalidades
- Garanta que testes existentes continuam passando
- Mantenha cobertura de testes razoável

## Dúvidas?

- Converse com os outros membros do grupo
- Revise a documentação existente

## Obrigado!

Sua contribuição é valiosa para o projeto. Muito obrigado por contribuir! 🎉

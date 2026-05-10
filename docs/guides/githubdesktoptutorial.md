# **4 - GitHub Desktop**

Este guia ensina como usar o **GitHub Desktop** desde a adição de um repositório até fazer commits com co-autores e associar issues.

## Índice

1. [Instalação](#instalação)
2. [Configuração Inicial](#configuração-inicial)
3. [Adicionando um Repositório](#adicionando-um-repositório)
4. [Trabalhando com Branches](#trabalhando-com-branches)
5. [Fazendo Commits](#fazendo-commits)
6. [Relacionando Issues ao Commit](#relacionar-issues-ao-commit)
7. [Commits com Co-autores](#commits-com-co-autores)
8. [Associando Issues aos Commits](#associando-issues-aos-commits)
9. [Push e Pull](#push-e-pull)
10. [Dicas e Boas Práticas](#dicas-e-boas-práticas)

---

## Instalação

### Passo 1: Baixar GitHub Desktop

1. Acesse [desktop.github.com](https://desktop.github.com)
2. Clique em **"Download for Windows"** (ou a opção para seu sistema operacional)
3. Execute o instalador e siga as instruções

### Passo 2: Fazer Login

1. Abra o GitHub Desktop
2. Clique em **"Sign in"** no canto superior esquerdo
3. Selecione **"GitHub.com"** como sua opção de login
4. Você será redirecionado para o navegador para autenticar
5. Autorize o GitHub Desktop a acessar sua conta
6. Retorne ao GitHub Desktop e confirme o login

---

## Configuração Inicial

### Definir Identidade do Git

Antes de fazer commits, você precisa configurar seu nome e email:

1. Clique em **File** (Windows) ou **GitHub Desktop** (Mac) no menu
2. Selecione **Options** (Windows) ou **Preferences** (Mac)
3. Vá para a aba **Git**
4. Preencha:
   - **Name**: Seu nome completo
   - **Email**: Seu email associado ao GitHub
5. Clique em **Save**

---

## Adicionando um Repositório

### Opção 1: Clonar um Repositório Remoto

1. Clique em **File** → **Add** → **Clone repository**
2. Escolha a aba **GitHub.com** (se o repositório estiver no GitHub)
3. Selecione o repositório da lista ou procure pelo nome
4. Escolha o **Local path** (onde deseja salvar a pasta no seu computador)
5. Clique em **Clone**

**Exemplo:**
```
Repositório: fcte-pi1/2026_1_PI1_Grupo02_Juliana
Local path: C:\Users\seu-usuario\Documents\Projetos\grupo02
```

![Caminho para clonar repositório](../img/caminho%20para%20clonar%20repositorio.png)
*Figura: Interface de clonagem de repositório no GitHub Desktop*

![URL do GitHub](../img/url%20do%20github.png)
*Figura: Adicionando o link do repositório*

### Opção 2: Adicionar um Repositório Local Existente

1. Clique em **File** → **Add Local Repository**
2. Navegue até a pasta do seu repositório
3. Selecione-a e clique em **Select Folder**
4. Se não for um repositório Git válido, o GitHub Desktop oferecerá criar um novo

---

## Trabalhando com Branches

### Criar uma Nova Branch

1. Clique no botão **Current Branch** no topo da janela
2. Clique em **New Branch**
3. Preencha:
   - **Name**: Nome descritivo (ex: `feat/autenticacao`)
   - **From branch**: Selecione a branch base (geralmente `main` ou `Pages`)
4. Clique em **Create Branch**
5. Clique em **Publish branch** para enviar ao repositório remoto

### Trocar de Branch

1. Clique em **Current Branch**
2. Selecione a branch desejada da lista
3. Clique em **Switch to Branch**

### Deletar uma Branch

1. Clique em **Current Branch**
2. Clique com botão direito na branch que deseja deletar
3. Selecione **Delete**

### Aviso
    Sempre certifique-se de que suas mudanças foram enviadas (push) antes de deletar uma branch!


## Fazendo Commits

### Passo 1: Fazer Alterações nos Arquivos

Edite seus arquivos usando seu editor favorito (VS Code, Sublime Text, etc).

### Passo 2: Revisar Mudanças

1. No GitHub Desktop, você verá a aba **Changes** automaticamente selecionada
2. Na seção **Changes**, você verá:
   - **Modified files**: Arquivos alterados
   - **Staged files**: Arquivos prontos para commit
   - **Untracked files**: Arquivos novos ainda não rastreados

### Passo 3: Selecionar Arquivos para Stage

1. Marque os checkboxes dos arquivos que deseja incluir no commit
2. Os arquivos selecionados aparecerão em **Staged**

### Dica
    Se precisar revisar as alterações específicas de um arquivo, clique nele para ver o diff (diferenças) na janela direita.

### Passo 4: Escrever a Mensagem de Commit

1. No campo **Summary**, escreva uma mensagem breve e clara (máx. 50 caracteres)
2. Opcionalmente, adicione uma descrição mais detalhada no campo **Description**
3. Exemplos de boas mensagens:
   ```
   feat: adicionar autenticação com Google
   fix: corrigir bug no formulário de login
   docs: atualizar README com instruções de instalação
   refactor: reorganizar estrutura de pastas
   ```

### Passo 5: Relacionar Issues ao Commit

Para associar seu commit a uma ou mais issues, adicione referências no campo **Description** usando a `#` seguida do número da issue:

**Como fazer:**

1. No campo **Description**, após descrever suas mudanças, adicione uma linha em branco
2. Digite uma das palavras-chave seguidas pelo número da issue:
   - `Fixes #42` - Fecha a issue automaticamente quando o commit for mesclado
   - `Closes #42` - Também fecha a issue
   - `Resolves #42` - Também fecha a issue
   - `Relacionado com #42` - Apenas menciona, não fecha

**Exemplo completo:**

```
Summary: corrigir validação de email

Description:
Corrigido o validador de email que estava aceitando
caracteres especiais inválidos. Adicionada validação
mais robusta usando regex.

Fixes #15
Closes #18
```

**Resultado:**
- O commit será linkado automaticamente às issues #15 e #18
- Quando o código for mesclado à branch principal, as issues serão fechadas automaticamente

![Adição de Arquivos Modificados e Relacionando a Issue](../img/adicionando%20arquivos%20e%20relacionando%20issue.png)
*Figura: Exemplo de commit com relacionamento de issues*

### Dica
    Você pode mencionar múltiplas issues no mesmo commit. Basta adicionar uma linha para cada uma!

---

## Commits com Co-autores

### O que é um Commit com Co-autor?

Um commit com co-autor é uma forma de atribuir crédito a múltiplos contribuidores pelo mesmo commit. Isso é importante em trabalhos em grupo!

### Como Adicionar Co-autores

#### Método 1: Usar a Interface do GitHub Desktop

1. Escreva a mensagem de commit normalmente
2. Clique em **Commit details** (você verá um ícone ou menu expandido)
3. Procure por **Co-authors** ou **Add co-author**
4. Clique para adicionar um co-autor
5. Procure pelo usuário GitHub do co-autor
6. Selecione-o da lista
7. Faça o commit

![Descrição do Commit e Co-autor](../img/Descri%C3%A7%C3%A3o%20do%20commit%20e%20co-autor.png)
*Figura: Campo de descrição do commit com co-autores no GitHub Desktop*


### Importante
    Os co-autores precisam ter uma conta GitHub. Use o nome de usuário e email associados à conta deles.

---

## Push e Pull

### Push: Enviar Commits para o Servidor

1. Clique no botão **Push origin** no topo da janela
2. Aguarde o envio completar
3. Você verá uma confirmação de sucesso

### Dica
    Se você criou uma nova branch, será necessário clicar em **Publish branch** antes de fazer push.

### Pull: Atualizar do Servidor

1. Clique no botão **Fetch origin** para buscar atualizações
2. Se houver atualizações, clique em **Pull origin**
3. Os commits remotos serão mesclados ao seu repositório local

---

## Dicas e Boas Práticas

### O que Fazer

- **Commits frequentes**: Faça commits pequenos e bem-definidos
- **Mensagens claras**: Use mensagens descritivas e em tempo presente
- **Uma tarefa por commit**: Cada commit deve resolver uma issue ou implementar um recurso
- **Sempre faça pull antes de push**: Evita conflitos
- **Use branches**: Desenvolva em branches separadas, nunca na `main`
- **Revise antes de commitar**: Sempre veja o diff antes de commitar

### O que Evitar

- **Commits gigantes**: Múltiplas tarefas em um só commit
- **Mensagens vagas**: "Ajustes", "Correções", "Update"
- **Commits diretamente na main**: Sempre use branches
- **Push sem revisar**: Sempre revise as mudanças
- **Esquecer de fazer pull**: Pode causar conflitos desnecessários

### Fluxo Recomendado de Trabalho

```
1. Fetch origin (atualizar)
2. Pull origin (puxar mudanças)
3. Create new branch (criar branch feature)
4. Make changes (fazer alterações)
5. Stage files (selecionar arquivos)
6. Commit with description (commitar com descrição clara)
7. Push origin (enviar commits)
8. Create Pull Request no GitHub
9. Peer review
10. Merge para main
```

---

## Recursos Adicionais

- [Documentação Oficial do GitHub Desktop](https://docs.github.com/en/desktop)
- [GitHub Flow - Fluxo Recomendado](https://guides.github.com/introduction/flow/)
- [Conventional Commits](https://www.conventionalcommits.org/pt-br/)
- [Como Escrever Boas Mensagens de Commit](https://chris.beams.io/posts/git-commit/)

---

**Última atualização:** 2026
**Autor:** Pedro Luciano

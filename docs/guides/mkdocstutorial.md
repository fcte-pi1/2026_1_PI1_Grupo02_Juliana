
# **3 - Usando o MkDocs:**

## **Rodar o MkDocs localmente**

Esse passo serve para você conseguir acessar em tempo real as mudanças que você fizer no conteúdo do Pages localmente, antes de commitar no Github.

Dentro da env navegue até a pasta do projeto.

```bash
cd 2026_1_PI1_Grupo02_Juliana
```

Para rodar servidor de desenvolvimento use:

```bash
mkdocs serve --livereload
```
O site estará disponível para ser visualizado no navegador através do link na imagem: 
![pages](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image9_a1r3ag)

---

Para criar um arquivo no MkDocs, siga o procedimento padrão de criação de arquivos, adicionando a extensão **.md**. Todas as edições devem ser realizadas em arquivos com essa mesma extensão. Abaixo um exemplo de nomenclatura:

![Exemplo](https://res.cloudinary.com/dqeji5jup/image/upload/f_auto,q_auto/image22_e0qcms)

Um arquivo .md deve ser escrito utilizando Markdown. 

- [Guia rápido de Markdown.](https://markdownlivepreview.com/)

Para que o novo arquivo seja visualizado no pages você precisa adicionar o nome do arquivo criado na lista de navegação em **"mkdocs.yml"**. A organização depende do conteúdo do arquivo: se for um guia, adicione na aba correspondente, se não, crie uma nova categoria.

```bash
nav:
  - Home: index.md
  -    <<< Pode ser adicionado aqui  
  - Guias:
      - Como Começar: guides/start.md
      - Como usar LaTeX: guides/latextutorial.md
      - Como usar MkDocs: guides/mkdocstutorial.md
      - Como Contribuir: guides/contributing.md
      - Referência Rápida: guides/quick-reference.md
      -   <<< Ou adicionado aqui
```

## Checklist

- [ ] `mkdocs serve` rodando em http://localhost:8000
- [ ] Site carregando corretamente

---

## Próximas ações

1. Leia o [guia de contribuição](contributing.md).
2. Consulte a [referência rápida](quick-reference.md) quando precisar.
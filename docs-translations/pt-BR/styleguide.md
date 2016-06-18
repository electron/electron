# Styleguide do Electron

Localize a seção apropriada para a sua tarefa: [Lendo a documentação do Electron](#)
ou [Escrevendo documentação para o Electron](#).

## Escrevendo documentação para o Electron

Estas são as formas que escrevemos a documentação do Electron.

- No Máximo um `h1` por página.
- Usar `bash` ao invés de` cmd` em blocos de código (por causa do syntax highlighter).
- Títulos `h1`  deve coincidir com o nome do objeto (i.e. `browser-window` →
`BrowserWindow`).
- Nomes de arquivos separados por hífen.
- Adicionar pelo menos uma descrição a cada frase.
- Métodos de cabeçalhos são envolto em `code`.
- Cabeçalhos de eventos são envolto em single 'quotation' marks.
- Não há listas com identação com mais de 2 níveis (por causa do markdown).
- Adicione títulos nas seções: Events, Class Methods e Instance Methods.
- Use 'will' ao invéis de 'would' ao descrever os resultados.
- Eventos e métodos são com cabeçalhos `h3`.
- Argumentos opcionais escritos como  `function (required[, optional])`.
- Argumentos opcionais são indicados quando chamado na lista.
- Comprimento da linha é de 80 caracteres com colunas quebradas.
- Métodos específicos para uma plataforma são postos em itálico seguindo o cabeçalho do método.
 - ```### `method(foo, bar)` _macOS_```

## Lendo a documentação do Electron

Aqui estão algumas dicas de como entender a sintaxe da documentacão do Electron.

### Métodos

Um exemplo de [method](https://developer.mozilla.org/en-US/docs/Glossary/Method)
documentação:

---

`methodName(required[, optional]))`

* `require` String, **required**
* `optional` Integer

---

O nome do método é seguido pelos seus argumentos. Argumentos opcionais são
simbolizada por colchetes que cercam o argumento opcional, bem como a vírgula
requerido se este argumento opcional segue outro argumento.

Abaixo o método é para obter informações detalhadas sobre cada um dos argumentos. O tipo
de argumento é simbolizada por qualquer um dos tipos mais comuns: [`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String), [` Number`](https : //developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number), [`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript Referência/Global_Objects/Object), [`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
ou um tipo personalizado como o de Electron [`webContent`](api/web-content.md).

### Eventos

Um exemplo de [evento](https://developer.mozilla.org/en-US/docs/Web/API/Event)
documentação:

---

Event: 'wake-up'

Returns:

* `time` String

---

O evento é uma cadeia que é utilizada após um `.on` em um método listner. Se ela retorna
-lhe um valor e seu tipo é observado abaixo. Se você quiser um método para esctuar e responder
crie algo parecido com o exemplo abaixo:

```javascript
Alarm.on('wake-up', function(time) {
  console.log(time)
})
```
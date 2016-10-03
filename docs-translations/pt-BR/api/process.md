# process
O objeto `process` no Electron tem as seguintes diferenças do objeto no upstream node:

* `process.type` String - Tipo de processo, pode ser `browser` (processo principal) 
ou `renderer`.
* `process.versions.electron` String - Versão do Electron.
* `process.versions.chrome` String - Versão do Chromium.
* `process.resourcesPath` String - Caminho para o código fonte JavaScript.
* `process.mas` Boolean - Para build da Mac App Store, este valor é `true`, para outros builds é `undefined`.

## Eventos

### Evento: 'loaded'

Emitido quando o Electron carregou seu script de inicialização interno e está começando a carregar a página web ou o script principal.

Pode ser utilizado pelo script pré-carregamento (preload.js abaixo) para adicionar símbolos globais do Node removidos para o escopo global quando a integração do node é desligada:

```js
// preload.js
var _setImmediate = setImmediate
var _clearImmediate = clearImmediate
process.once('loaded', function () {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})
```

## Propriedades

### `process.noAsar`

Definir isto para `true` pode desabilitar o suporte para arquivos `asar` nos módulos nativos do Node.

# Métodos

O objeto `process` tem os seguintes métodos:

### `process.hang`

Faz com que o *thread* principal do processo congele.

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

Define o limite do arquivo descritor para `maxDescriptors` ou para o limite do OS, 
o que for menor para o processo atual.

# Processo

O  objeto `process` em Electron tem as seguintes diferenças de um nó upstream :

* `process.type` String - Tipo de processo, pode ser `browser` (i.e. main process)
  ou `renderer`.
* `process.versions['electron']` String - Versão do Electron.
* `process.versions['chrome']` String - Versão do Chromium.
* `process.resourcesPath` String - Caminho para Código JavaScript.

# Métodos

O objeto `process` tem o seguinte método:

### `process.hang`

Causa a principal thread do processo atual pendurar.

## process.setFdLimit(MaxDescritores) _OS X_ _Linux_

* `MaxDescritores` Inteiro (Integer)

Setar o limite do arquivo descritor para `MaxDescritores`ou o limite do OS,
o que for menor para o processo atual.
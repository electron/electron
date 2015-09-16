# process
O objeto `process` no Electron tem as seguintes diferenças de um upstream node:

* `process.type` String - Tipo de processo, pode ser `browser` (i.e. main process) 
ou `renderer`.
* `process.versions['electron']` String - Versão do Electron.
* `process.versions['chrome']` String - Versão do Chromium.
* `process.resourcesPath` String - Caminho para os códigos fontes JavaScript.

# Métodos
O objeto `process` tem os seguintes método:

### `process.hang`

Afeta a thread principal do processo atual.

## process.setFdLimit(MaxDescritores) _OS X_ _Linux_

* `maxDescriptors` Integer

Define o limite do arquivo descritor para `maxDescriptors` ou para o limite do OS, 
o que for menor para o processo atual.
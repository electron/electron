# Empacotamento da aplicação

Para proteger os recursos e o código fonte da sua aplicação você pode optar por
empacotar a sua aplicação em um arquivo [asar](https://github.com/atom/asar), isto é possível com poucas
alterações em seu código.

## Gerando um arquivo `asar`

Um arquivo [asar][asar] é um formato parecido com tar ou zip bem simples que concatena arquivos
em um único arquivo. O Electron pode ler arquivos arbitrários a partir dele sem descompacatar
o arquivo inteiro.

Passos para empacotar a sua aplicação em um arquivo `asar`:

### 1. Instale o utilitário asar

```bash
$ npm install -g asar
```

### 2. Empacote a sua aplicação

```bash
$ asar pack your-app app.asar
```

## Usando arquivos `asar`

No Electron existem dois conjuntos de APIs: Node APIs fornecidas pelo Node.js e Web
APIs fornecidas pelo Chromium. Ambas as APIs suportam a leitura de arquivos `asar`.

### Node API

As API's do Node como `fs.readFile` e `require` tratam os pacotes `asar`
como diretórios virtuais e os arquivos dentro dele como arquivos normais.

Por exemplo, temos um arquivo `example.asar` sob `/path/to`:

```bash
$ asar list /path/to/example.asar
/app.js
/file.txt
/dir/module.js
/static/index.html
/static/main.css
/static/jquery.min.js
```

Lendo um arquivo em pacote `asar`:

```javascript
var fs = require('fs');
fs.readFileSync('/path/to/example.asar/file.txt');
```

Listando todos os arquivos a partir da raiz:

```javascript
var fs = require('fs');
fs.readdirSync('/path/to/example.asar');
```

Utilizando um módulo dentro do pacote `asar`:

```javascript
require('/path/to/example.asar/dir/module.js');
```

Você também pode renderizar uma página web apartir de um arquivo `asar` utilizando o módulo `BrowserWindow`:

```javascript
var BrowserWindow = require('browser-window');
var win = new BrowserWindow({width: 800, height: 600});
win.loadURL('file:///path/to/example.asar/static/index.html');
```

### API Web

Em uma página web, arquivos em um pacote `asar` pode ser solicitado com o protocolo `file:`.
Como a API Node, arquivos `asar` são tratadas como diretórios.

Por exemplo, para obter um arquivo com `$ .get`:

```html
<script>
var $ = require('./jquery.min.js');
$.get('file:///path/to/example.asar/file.txt', function(data) {
  console.log(data);
});
</script>
```

### Tratando um pacote `asar` como um arquivo normal

Para alguns casos, precisamos verificar o checksum de um pacote `asar`, para fazer isto, precisamos ler
o arquivo `asar` como um arquivo normal. Para isto, você pode usar o built-in
`original-fs` que fornece a API `fs`, sem apoio a arquivos asar`:

```javascript
var originalFs = require('original-fs');
originalFs.readFileSync('/path/to/example.asar');
```

## Limitaçõs na API Node

Mesmo fazendo grandes esforços para pacotes `asar` ser tratado no Node como diretórios,
ainda existem limitações devido a natureza de baixo nível do Node

### Arquivos `asar` são somente leitura

Os arquivos `asar` não podem ser modificados.

### Diretório de trabalho não pode ser comportar como diretório de arquivos

Embora pacotes `asar` são tratadas como diretórios, não há
diretórios reais no sistema de arquivos, assim você nunca pode definir o diretório de trabalho para
diretórios em pacotes `asar`, passando-os como a opção `cwd` de algumas APIs
também irá causar erros.

### Descompactação extra em algumas APIs

A maioria das APIs `fs` pode ler um arquivo ou obter informações de um arquivo a partir de pacotes `asar`
sem descompacta-lo, mas para algumas APIs da rota real o Electron irá extrair o arquivo necessário para um
arquivo temporário e passar o caminho do arquivo temporário para as APIs,
isso adiciona um pouco de sobrecarga para essas APIs.

APIs que requer descompactação extras são:

* `child_process.execFile`
* `fs.open`
* `fs.openSync`
* `process.dlopen` - Usado por `require` em módulos nativos

### Falsas informações de status do módulo `fs.stat`

O objeto `Stats` retornado por` fs.stat` e outras funções relacionadas não são informações confiáveis, 
você não deve confiar no objeto `Stats` exceto para obter o
tamanho do arquivo e verificação de tipo de arquivo.

## Adicionando arquivos em um pacote `asar`

Como dito acima, algumas APIs deo Node irá descompactar o arquivo para quando o filesystem
requsistar, além dos problemas de desempenho, ele também pode levar a falsos alertas
de vírus.

Para contornar isso, você pode descompactar alguns arquivos usando a
opção `--unpack`, um exemplo de exclusão de bibliotecas compartilhadas de módulos nativos
é:

```bash
$ asar pack app app.asar --unpack *.node
```

Depois de executar o comando, além do `app.asar`, há também
`app.asar.unpacked` pasta gerada que contém os arquivos descompactados, você
deve copiá-lo juntamente com `app.asar` quando enviá-lo para os usuários.

Mais informações no repositório [asar](https://github.com/atom/asar)

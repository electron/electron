# Depurando o Processo Principal

A janela do navegador, DevTools, pode somente depurar o processo de renderização
de scripts (por exemplo, as páginas da web). Para providenciar um modo de
depurar os scripts através do processo principal, o Electron criou as opções
`--debug` e `--debug-brk`.

## Opções da Linha de Comando

Use a seguinte opção na linha de comando para depurar o processo principal do
Electron:

### `--debug=[porta]`

Quando este comando é usado, o Electron irá executar o protocolo de depuração
V8 mandando as mensagens na `porta`. A `porta` padrão é `5858`.

### `--debug-brk=[porta]`

Semelhante ao `--debug`, porém pausa o script na primeira linha.

## Usando node-inspector para depurar

__Nota:__ O Electron usa a versão v0.11.13 do Node, a qual, atualmenta não
funciona muito bem com node-inspector, e o processo principal irá quebrar se
você inspecionar o objeto `process` pelo console do node-inspector.

### 1. Inicie o servidor [node-inspector][node-inspector]

```bash
$ node-inspector
```

### 2. Habilite o modo de depuração para o Electron

Você pode também iniciar o Electron com um ponto de depuração, desta maneira:

```bash
$ electron --debug=5858 sua/aplicacao
```

ou para pausar o script na primeira linha:

```bash
$ electron --debug-brk=5858 sua/aplicacao
```

### 3. Carregue o debugger UI

Abra este endereço http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858
usando o Chrome.

[node-inspector]: https://github.com/node-inspector/node-inspector

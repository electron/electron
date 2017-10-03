# Variáveis de ambiente

> Controla as configurações e comportamento sem modificar o código

Certos comportamentos do Electron são controlados por variáveis de ambiente que
são inicializadas antes do código do aplicativo e da chamada da linha de comando.

Exemplo no POSIX shell:

```bash
$ export ELECTRON_ENABLE_LOGGING=true
$ electron
```

Exemplo no prompt de comando do Windows:

```powershell
> set ELECTRON_ENABLE_LOGGING=true
> electron
```

## Variáveis de produção

As seguintes variáveis de ambiente são destinadas principalmente para o uso
em tempo de execução, empacotadas nas aplicações Electron.

### `GOOGLE_API_KEY`

O Electron já vem com uma chave para API inclusa para fazer requisições para
o serviço de geolocalização do Google. Por causa da chave da API vir inclusa
em cada versão do Electron, frequentemente excede a cota de uso. Para contornar
isso, você pode informar sua própria chave da API do Google nas variáveis de
ambiente. Informe o código no seu arquivo principal, antes do browser
ser aberto, isso  fará com que a aplicação use-o para fazer as requisições:

```javascript
process.env.GOOGLE_API_KEY = 'YOUR_KEY_HERE'
```

Para maiores instruções de como conseguir uma chave da API do Google, visite
[essa página](https://www.chromium.org/developers/how-tos/api-keys).

Por padrão, a mais recente chave da API do Google pode não ser permitida para
fazer requisições de geolocalização. Para habilitar as requisições, acesse
[essa página](https://console.developers.google.com/apis/api/geolocation/overview).

### `ELECTRON_NO_ASAR`

Desabilita o suporte ASAR. Essa variável é suportada somente em processos
filhos bifurcados e gerados a partir de processos que definem
`ELECTRON_RUN_AS_NODE`.

### `ELECTRON_RUN_AS_NODE`

Inicia o processo como um processo padrão de Node.js.

### `ELECTRON_NO_ATTACH_CONSOLE` _Windows_

Não anexa a janela à sessão atual do console.

### `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

Não usa a barra de menu global do Linux.

## Variáveis de desenvolvimento

As seguintes variáveis de ambiente são destinadas principalmente ao
desenvolvimento e o processo de debug.


### `ELECTRON_ENABLE_LOGGING`

Imprime os logs internos do Chrome no terminal.

### `ELECTRON_LOG_ASAR_READS`

Quando o Electron lê um arquivo ASAR, registra o deslocamento e o caminho
do arquivo no sistema `tmpdir`. O arquivo resultante pode ser fornecido
ao módulo ASAR para otimizar a organização dos arquivos.

### `ELECTRON_ENABLE_STACK_DUMPING`

Imprime o stack trace no terminal quando o Electron trava.

Essa variável de ambiente não funcionará se o `crashReporter` for iniciado.

### `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

Exibe a janela de erro do Windows quando o Electron trava.

Essa variável de ambiente não funcionará se o `crashReporter` for iniciado.

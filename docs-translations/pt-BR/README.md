Por favor, certifique-se de que está utilizando a documentação que corresponde à sua versão do Electron.
O número da versão deve ser uma parte da URL da página. Se não for, você provavelmente está utilizando
a documentação de um branch de desenvolvimento que pode conter mudanças na API que não são compatíveis
com a sua versão do Electron. Se este for o caso, você pode mudar para uma versão diferente da
documentação na lista de [versões disponíveis](http://electron.atom.io/docs/) em atom.io,
ou se você estiver usando a interface do GitHub, abra o *dropdown* "Switch branches/tags" e
selecione a *tag* que corresponde à sua versão.

## FAQ

Existem muitas perguntas comuns que são feitas, verifique antes de criar uma issue.
* [Electron FAQ](faq.md)

## Guias

* [Plataformas Suportadas](tutorial/supported-platforms.md)
* [Distribuição de Aplicações](tutorial/application-distribution.md)
* [Guia de Submissão da Mac App Store](../../docs/tutorial/mac-app-store-submission-guide.md)
* [Empacotamento da Aplicação](tutorial/application-packaging.md)
* [Usando Módulos Nativos do Node](tutorial/using-native-node-modules.md)
* [Depuração do Processo Principal](tutorial/debugging-main-process.md)
* [Usando Selenium e WebDriver](../../docs/tutorial/using-selenium-and-webdriver.md)
* [Extensão DevTools](tutorial/devtools-extension.md)
* [Usando o Plugin Pepper Flash](tutorial/using-pepper-flash-plugin.md)
* [Usando o Plugin Widevine CDM](../../docs/tutorial/using-widevine-cdm-plugin.md)

## Tutoriais

* [Introdução](tutorial/quick-start.md)
* [Integração com o Ambiente de Desenvolvimento](tutorial/desktop-environment-integration.md)
* [Evento de Detecção Online/Offline](tutorial/online-offline-events.md)

## API - Referências

* [Sinopse](api/synopsis.md)
* [Processos](api/process.md)
* [Aceleradores (Teclas de Atalho)](api/accelerator.md)
* [Parâmetros CLI suportados (Chrome)](../../docs/api/chrome-command-line-switches.md)
* [Variáveis de Ambiente](../../docs/api/environment-variables.md)

### Elementos DOM Personalizados:

* [Objeto `File`](api/file-object.md)
* [Tag `<webview>`](../../docs/api/web-view-tag.md)
* [Função `window.open`](api/window-open.md)

### Módulos para o Processo Principal:

* [app](api/app.md)
* [autoUpdater](api/auto-updater.md)
* [BrowserWindow](api/browser-window.md)
* [contentTracing](../../docs/api/content-tracing.md)
* [dialog](../../docs/api/dialog.md)
* [globalShortcut](../../docs/api/global-shortcut.md)
* [ipcMain](../../docs/api/ipc-main.md)
* [Menu](../../docs/api/menu.md)
* [MenuItem](../../docs/api/menu-item.md)
* [powerMonitor](api/power-monitor.md)
* [powerSaveBlocker](../../docs/api/power-save-blocker.md)
* [protocol](../../docs/api/protocol.md)
* [session](../../docs/api/session.md)
* [webContents](../../docs/api/web-contents.md)
* [Tray](../../docs/api/tray.md)

### Módulos para o Processo Renderizador:

* [DesktopCapturer](../../docs/api/desktop-capturer.md)
* [ipcRenderer](../../docs/api/ipc-renderer.md)
* [remote](../../docs/api/remote.md)
* [webFrame](../../docs/api/web-frame.md)

### Módulos para ambos os processos:

* [clipboard](../../docs/api/clipboard.md)
* [crashReporter](../../docs/api/crash-reporter.md)
* [nativeImage](../../docs/api/native-image.md)
* [screen](../../docs/api/screen.md)
* [shell](api/shell.md)

## Desenvolvimento

* [Estilo de Código](development/coding-style.md)
* [Estrutura de Diretórios de Código Fonte](development/source-code-directory-structure.md)
* [Diferenças Técnicas do NW.js (antigo node-webkit)](../../docs/development/atom-shell-vs-node-webkit.md)
* [Visão Geral do Build](../../docs/development/build-system-overview.md)
* [Instrução de Build (Mac)](development/build-instructions-osx.md)
* [Instrução de Build (Windows)](../../docs/development/build-instructions-windows.md)
* [Instrução de Build (Linux)](development/build-instructions-linux.md)
* [Configurando um Symbol Server no Debugger](../../docs/development/setting-up-symbol-server.md)

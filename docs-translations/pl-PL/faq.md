# Electron - pytania i odpowiedzi

## Kiedy Electron zostanie zaktualizowany do najnowszej wersji Chrome?
Wersja Chrome Electron jest zazwyczaj aktualizowana co tydzień lub dwa, po wypuszczeniu nowej wersji Chrome. Ten czas jest luźny i zazwyczaj zależy od ilości czasu potrzebnej do zaktualizowania. 

Jedynie stabilne wersje Chrome są wykorzystywane. Jeżeli ważne poprawki są w beta lub rozwojowej wersji, zostaną one zaportowane. 

Po więcej informacji, proszę zapoznaj się z [informacjami bezpieczeństwa](tutorial/security.md).

## Kiedy Electron zostanie zaktualizowany do najnowszej wersji Node.js?
Wtedy, gdy nowa wersja Node.js zostanie wypuszczona. Zazwyczaj czekamy miesiąc przed aktualizacją w Electron. Dzięki temu unikamy ewentualnych problemów z bugami.

Nowe funkcje Node.js są zazwyczaj wprowadzane wraz z aktualizacjami V8. Odkąd Electron używa V8 dostarczanego przeglądarce Chrome, nowe funkcje Node.js są zazwyczaj dostępne w Electron.

## Jak przekazywać informacje pomiędzy stronami?
Aby przekazywać dane pomiędzy stronami (proces renderujący) najprostszą drogą będzie użycie części API HTML5, która jest dostarczana przeglądarkom. Mowa o
[Storage API][storage], [`localStorage`][local-storage],
[`sessionStorage`][session-storage] lub [IndexedDB][indexed-db].

Lub możesz użyć IPC system, który jest specyficzny dla Electron, aby przechowywać obiekty w głównym procesie jako zmienna globalna i dopiero wtedy uzyskiwać dostęp do niej z poziomu renderera przez właściwość `remote` z modułu `electron`:



```javascript
// W głównym procesie.
global.sharedObject = {
  someProperty: 'default value'
}
```

```javascript
// Na stronie pierwszej.
require('electron').remote.getGlobal('sharedObject').someProperty = 'new value'
```

```javascript
// Na stronie drugiej.
console.log(require('electron').remote.getGlobal('sharedObject').someProperty)
```

## Okno/ikona w zasobniku mojej aplikacji nie pojawia się po kilku minutrach.
Może się tak dziać, gdy zmienna przechowująca dane o oknie/ikonie z zasobnika zostanie pochłonięta przez garbage collector.

Jeżeli zauważyłeś ten problem, poniższe artykuły mogą okazać się pomocne:
* [Memory Management][memory-management]
* [Variable Scope][variable-scope]

Jeżeli chcesz to szybko naprawić, możesz zrobić zmienne globalną zmieniając swój kod z tego:
```javascript
const {app, Tray} = require('electron')
app.on('ready', () => {
  const tray = new Tray('/path/to/icon.png')
  tray.setTitle('hello world')
})
```

na to:

```javascript
const {app, Tray} = require('electron')
let tray = null
app.on('ready', () => {
  tray = new Tray('/path/to/icon.png')
  tray.setTitle('hello world')
})
```

## I can not use jQuery/RequireJS/Meteor/AngularJS in Electron.

Due to the Node.js integration of Electron, there are some extra symbols
inserted into the DOM like `module`, `exports`, `require`. This causes problems
for some libraries since they want to insert the symbols with the same names.

To solve this, you can turn off node integration in Electron:

```javascript
// In the main process.
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
})
win.show()
```

But if you want to keep the abilities of using Node.js and Electron APIs, you
have to rename the symbols in the page before including other libraries:

```html
<head>
<script>
window.nodeRequire = require;
delete window.require;
delete window.exports;
delete window.module;
</script>
<script type="text/javascript" src="jquery.js"></script>
</head>
```

## `require('electron').xxx` is undefined.

When using Electron's built-in module you might encounter an error like this:

```
> require('electron').webFrame.setZoomFactor(1.0)
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

This is because you have the [npm `electron` module][electron-module] installed
either locally or globally, which overrides Electron's built-in module.

To verify whether you are using the correct built-in module, you can print the
path of the `electron` module:

```javascript
console.log(require.resolve('electron'))
```

and then check if it is in the following form:

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

If it is something like `node_modules/electron/index.js`, then you have to
either remove the npm `electron` module, or rename it.

```bash
npm uninstall electron
npm uninstall -g electron
```

However if you are using the built-in module but still getting this error, it
is very likely you are using the module in the wrong process. For example
`electron.app` can only be used in the main process, while `electron.webFrame`
is only available in renderer processes.

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API

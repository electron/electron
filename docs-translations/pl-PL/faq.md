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

## Nie mogę używać jQuery/RequireJS/Meteor/AngularJS w Electron.
W związku z integracją Node.js z Electron, jest kilka specjalnych symboli wykorzystywanych w DOM jak `module`, `exports`, `require`. W związku z tym niektóre biblioteki mogą stosować to samo nazewnictwo.

Aby rozwiązać ten problem, możesz wyłączyć integrację node w Electron. 
```javascript
// W głównym procesie.
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
})
win.show()
```

Jeżeli jednak chcesz zachować możliwość używania API zarówno Node.js jak i Electron, musisz zmienić nazwę symboli wykorzystywanych jeszcze przed załadowaniem innych bibliotek. 
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

Jeżeli używasz wbudowanych w Electron modułów możesz otrzymać błąd jak ten:

```
> require('electron').webFrame.setZoomFactor(1.0)
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

Jest to spowodowane tym, że  [npm `electron` moduł][electron-module], który jest zainstalowany lokalnie lub globalnie nadpisuje wbudowany moduły Electron.

Aby zweryfikować czy poprawnie czy poprawnie używasz wbudowanych modułów, możesz wypisać ścieżkę modułu `electron`. 

```javascript
console.log(require.resolve('electron'))
```

i wtedy sprawdzić następującą czy wygląda w następujący sposób.

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

Jeżeli ścieżka wygląda mniej więcej tak `node_modules/electron/index.js` to znaczy, że musisz usunąć npm moduł `electron` lub zmienić jego nazwę. 

```bash
npm uninstall electron
npm uninstall -g electron
```

Jeżeli mimo powyższych porad nadal otrzymujesz błąd, to bardzo prawdopodobne, że używasz modułu w złym procesie. Na przykład `electron.app` może być użyty jedynie w głównym procesie, podczas gdy `electron.webFrame` jest dostępny jedynie w procesie renderującym. 

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API

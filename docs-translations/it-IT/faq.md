# Electron FAQ

## Quando verrà utilizzata l'ultima versione di Chrome per Electron?

La versione di Chrome usata da Electron viene aggiornata solitamente entro una o
due settimane dal rilascio di una versione stabile di Chrome. Questa stima non è
garantita e dipende dall'ammontare di lavoro necessario all'aggiornamento.

E' utilizzato solo il canale stabile di Chrome. Se un fix importante si trovasse
sui canali beta o dev, lo applicheremo a una versione precedente.

Per maggiori informazioni, leggi l'[introduzione alla sicurezza](tutorial/sicurezza.md).

## Quando verrà utilizzata l'ultima versione di Node.js per Electron?

Quando una nuova versione di Node.js viene rilasciata, aspettiamo per circa un
mese prima di aggiornare quella di Electron. Possiamo così evitare di essere
influenzati dai bug introdotti nelle nuove versioni di Node.js, cosa che accade
spesso.

Le nuove funzionalità di Node.js sono solitamente introdotte dagli aggiornamenti
di V8. Siccome Electron usa la versione di V8 incorporata nel browser Chrome, le
nuove funzionalità di JavaScript implementate nella nuova versione di Node.js
sono già presenti in Electron.

## Come condividere dati tra pagine web?

Il modo più semplice per condividere dati tra pagine web (il processo di 
rendering) è usare le API di HTML5 già disponibili nei browser. Buone opzioni
sono [Storage API][storage], [`localStorage`][local-storage], [`sessionStorage`][session-storage] e [IndexDB][indexed-db].

Oppure puoi usare il sistema IPC, che è specifico di Electron, per memorizzare
gli oggetti nel processo principale come variabile globale e accedervi poi
dai renderer tramite la proprietà `remote` del modulo `electron`:

```javascript
// Nel processo principale.
global.sharedObject = {
  someProperty: 'valore di default'
}
```

```javascript
// Nella pagina 1.
require('electron').remote.getGlobal('sharedObject').someProperty = 'nuovo valore'
```

```javascript
// Nella pagina 2.
console.log(require('electron').remote.getGlobal('sharedObject').someProperty)
```

## La finestra/icona della mia app è sparita dopo qualche minuto.

Ciò accade quando una variabile usata per la memorizzazione della finestra/
icona viene garbage-collected.

Se dovessi incontrare questo problema, i seguenti articoli potrebbero esserti
d'aiuto:

* [Gestione Memoria][memory-management]
* [Visibilità Variabili][variable-scope]

Se hai bisogno un fix veloce, puoi rendere le variabili globali cambiando il tuo
codice da così:

```javascript
const {app, Tray} = require('electron')
app.on('ready', () => {
  const tray = new Tray('/percorso/icona.png')
  tray.setTitle('ciao mondo')
})
```

a così:

```javascript
const {app, Tray} = require('electron')
let tray = null
app.on('ready', () => {
  tray = new Tray('/percorso/icona.png')
  tray.setTitle('ciao mondo')
})
```

## Non posso usare jQuery/RequireJS/Meteor/AngularJS in Electron.

Data l'integrazione di Node.js di Electron, vi sono alcuni simboli extra
inseriti nel DOM quali `module`, `exports`, `require`. Ciò causa problemi ad
alcune librerie in quanto vogliono inserire simboli con gli stessi nomi.

Per risolvere il problema, puoi disattivare l'integrazione di Node in Electron:

```javascript
// Nel processo principale.
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
})
win.show()
```

Se invece volessi mantenere la capacità di usare Node.js e le API di Electron,
devi rinominare i simboli nella pagina prima di includere altre librerie:

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

Mentre usi il modulo integrato di Electron potresti incorrere in un errore
come questo:

```
> require('electron').webFrame.setZoomFactor(1.0)
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

Ciò è causato dal fatto che hai il [modulo npm `electron`][electron-module]
installato localmente o globalmente, e ciò sovrascrive il modulo integrato di
Electron.

Per verificare che tu stia usando il modulo integrato corretto, puoi stampare a
schermo il percorso del modulo `electron`:

```javascript
console.log(require.resolve('electron'))
```

e dopodiché controlla che sia in questa forma

```
"/percorso/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

Se dovesse essere simile a `node_modules/electron/index.js`, allora devi
rimuovere il modulo npm `electron` oppure rinominarlo.

```bash
npm uninstall electron
npm uninstall -g electron
```

Tuttavia, se usi il modulo integrato e continui a ricevere questo errore è molto
probabile che tu stia usando il modulo in un processo sbagliato. Per esempio
`electron.app` può essere usato solo nel processo principale, mentre
`electron.webFrame` è disponibile solo nel processo di rendering.

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API

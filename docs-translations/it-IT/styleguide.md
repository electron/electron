# Guida allo Stile della Documentazione

Queste sono le linee guida per la stesura della documentazione di Electron.

## Titoli

* Ogni pagina deve avere un singolo titolo di livello `#` all'inizio.
* I capitoli di una stessa pagina devono avere titoli di livello `##`.
* I sotto-capitoli devono avere un numero crescente di `#` a seconda del loro
livello di annidamento.
* Tutte le parole nel titolo della pagina devono iniziare con la lettera
maiuscola, a eccezione delle congiunzioni come "di" ed "e".
* Solo la prima lettera della prima parola di un capitolo deve essere maiuscola.

Prendendo `Guida Rapida` come esempio:

```markdown
# Guida Rapida

...

## Processo principale

...

## Processo di rendering

...

## Esegui la tua app

...

### Crea una distribuzione

...

### Electron scaricato manualmente

...
```

Esistono eccezioni a queste regole per quanto riguarda la documentazione delle
API.

## Regole markdown

* Usa `bash` invece di `cmd` nei blocchi di codice (per via della diversa
evidenziazione della sintassi).
* Le linee devono essere lunghe al massimo 80 caratteri.
* Non annidare le liste per più di due livelli (per via del rendering compiuto
da markdown).
* Tutti i blocchi di codice `js` o `javascript` sono analizzati con
[standard-markdown](http://npm.im/standard-markdown). 

## Documentazione API

Le regole seguenti vengono applicate solo alla documentazione delle API.

### Titolo della pagina

Ogni pagina deve avere come titolo il nome dell'oggetto a cui si riferisce
seguito da `require('electron')`, come ad esempio `BrowserWindow`, `autoUpdater`
e `session`.

Sotto il titolo della pagina deve esserci una descrizione della lunghezza di una
linea che comincia con `>`.

Prendendo `session` come esempio:

```markdown
# session

> Gestisce le sessioni browser, cookies, cache, impostazioni proxy, etc.
```

### Metodi ed eventi dei moduli

Per i moduli che non sono classi, i loro metodi ed eventi devono essere elencati
sotto i capitoli `## Metodi` ed `## Eventi`.

Prendendo `autoUpdate` come esempio:

```markdown
# autoUpdater

## Eventi

### Evento: 'error'

## Metodi

### `autoUpdater.setFeedURL(url[, requestHeaders])`
```

### Classi

* Le classi API e le classi che sono parte di moduli devono essere elencate
sotto un capitolo `## Classe: NomeDellaClasse`.
* Una pagina può avere più classi.
* I costruttoi devono essere elencati con un titolo di livello `###`.
* I [Metodi Statici](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes/static) (Inglese) devono essere elencati sotto un capitolo
`### Metodi Statici`.
* I [Metodi di Istanza](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes#Prototype_methods) (Inglese) devono essere elencati sotto un
capitolo `### Metodi di Istanza`.
* Gli Eventi di Istanza devono essere elencati sotto un capitolo
`## Eventi di Istanza`.
* Le Proprietà di Istanza devono essere elencate sotto un capitolo `## Proprietà di Istanza`.

Prendendo le classi `Session` e `Cookies` come esempi:

```markdown
# session

## Metodi

### session.fromPartition(partition)

## Proprietà

### session.defaultSession

## Classe: Session

### Eventi di Istanza

#### Evento: 'will-download'

### Metodi di Istanza

#### `ses.getCacheSize(callback)`

### Proprietà di Istanza

#### `ses.cookies`

## Classe: Cookies

### Metodi di Istanza

#### `cookies.get(filter, callback)`
```

### Metodi

Il capitolo dei metodi deve seguire il seguente formato:

```markdown
### `objectName.methodName(required[, optional]))`

* `required` String
* `optional` Integer (optional)

...
```

Il titolo può essere di livello `###` o `####` a seconda che sia un metodo di
un modulo o di una classe.

Per i moduli, il `nomeOggetto` è il nome del modulo. Per le classi, deve essere
il nome dell'istanza della classe e non deve essere lo stesso del modulo.

Per esempio, i metodi della classe `Session` sotto il modulo `session` devono
usare `ses` come `nomeOggetto`.

I parametri opzionali sono caratterizzati sia dalle parentesi quadre `[]` che
circondano il parametro, sia dalla virgola obbligatoria in caso il parametro
ne segua un altro.

```
required[, optional]
```

Sotto ogni metodo si trovano informazioni dettagliate su ogni parametro. Il tipo
di parametro è caratterizzato da uno dei tipi di dati comuni:

* [`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)
* [`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number)
* [`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)
* [`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
* [`Boolean`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)
* O da un tipo di dati personalizzato come [`WebContent`](api/web-content.md) di
Electron

Se un parametro o un metodo sono limitati a certe piattaforme, esse sono
segnalate, dopo il tipo di dato, attraverso l'uso di una lista di elementi in
corsivo e delimitati da uno spazio. I valori possono essere `macOS`,
`Windows` e `Linux`.

```markdown
* `animate` Boolean (optional) _macOS_ _Windows_
```

I parametri di tipo `Array` devono specificare nella descrizione sottostante
che elementi può contenere l'array.

La descrizione per i parametri di tipo `Funzione` dovrebbero rendere chiaro come
sia possibile chiamarli, ed elencare i tipi di parametri che vi saranno passati.

### Eventi

Il capitolo degli eventi deve seguire il seguente formato:

```markdown
### Evento: 'wake-up'

Ritorna:

* `time` String

...
```

Il titolo può essere di livello `###` o `####` a seconda che sia di un evento di
un modulo o di una classe.

I parametri di un evento seguono le stesse regole di quelli dei metodi.

### Proprietà

Il capitolo delle proprietà deve seguire il seguente formato:

```markdown
### session.defaultSession

...
```

Il titolo può essere di livello `###` o `####` a seconda che sia una proprietà
di un metodo o di una classe.

## Traduzioni della Documentazione

Le traduzioni della documentazione di Electron si trovano nella cartella
`docs-translations`.

Per aggiungere un altro set (o set parziale):

* Crea una sottocartella che abbia come nome il codice della lingua.
* Traduci i file.
* Aggiorna il file README.md dentro la cartella della lingua in modo che
reindirizzi ai file che hai tradotto.
* Aggiungi un collegamento alla cartella della traduzione nel [README](https://github.com/electron/electron#documentation-translations) principale di Electron.

Nota che tutti i file nella cartella `docs-translations` devono includere solo
i file tradotti. I file originali in inglese non devono essere copiati lì.

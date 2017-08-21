# Démarrage Rapide

Electron vous permet de créer des applications de bureau avec du JavaScript
fournissant un runtime avec des API riches natives (système d'exploitation).
Vous pourriez le voir comme une variante d'un Node.js directement exécutable
sur le bureau au lieu des serveurs Web.

Cela ne signifie pas que Electron est une liaison JavaScript à l'interface
utilisateur graphique (GUI). Au lieu de cela, Electron utilise des pages
Web comme GUI, donc vous pouvez aussi le voir comme un navigateur minimal
Chromium, contrôlé par JavaScript.

### Processus principal

Dans Electron, le processus qui exécute le script `main` de` package.json`
est appelé __le processus principal__. Le script qui s'exécute dans le
processus principal peut afficher une interface graphique en créant des
pages Web.

### Processus de rendu

Puisque Electron utilise Chromium pour afficher des pages Web, Chromium
Multi-process architecture est également utilisé. Chaque page Web d'Electron
fonctionne avec son propre processus, qui est appelé __le processus de rendu__.

Dans les navigateurs normaux, les pages Web sont habituellement exécutées
dans un environnement aux ressources indépendantes. Les utilisateurs d'électrons
ont cependant le pouvoir d'utiliser les API Node.js dans des pages Web permettant
un système d'exploitation de niveau inférieur d'interactions.

### Différences entre le processus principal et le processus de rendu

Le processus principal crée des pages Web en créant des instances `BrowserWindow`.
Chaque instance `BrowserWindow` exécute la page Web dans son propre processus
de rendu. Lorsqu'une occurrence `BrowserWindow` est détruite, le processus
de rendu correspondant est également terminé.

Le processus principal gère toutes les pages Web et leur processus rendu correspondant.
Chaque processus de rendu est isolé et ne se soucie que de la page Web en cours
d'exécution.

Dans les pages Web, l'appel des API relatives aux GUI natives n'est pas autorisé
car la gestion des ressources natives GUI dans les pages Web est très dangereuse,
il est facile de perdre des ressources. Si vous souhaitez effectuer des opérations
GUI dans une page Web, le Processus de la page Web doit communiquer avec le
processus principal pour lui demander d'effectuer ces opérations.

Dans Electron, nous avons plusieurs façons de communiquer entre le processus principal et
le processeurs. Comme [`ipcRenderer`](../api/ipc-renderer.md) et [`ipcMain`](../api/ipc-main.md) pour envoyer des messages, et les [Remote](../api/remote.md)
pour la communication de style RPC. Il y a aussi une entrée de FAQ sur
[comment partager des données entre des pages Web][share-data].

## Écrivez votre première application Electron

Généralement, une application Electron est structurée comme ceci :

```text
your-app/
├── package.json
├── main.js
└── index.html
```

Le format de `package.json` est exactement le même que celui des modules de Node, et
le script spécifié par le champ `main` est le script de démarrage de votre application,
qui exécutera le processus principal. Un exemple de votre `package.json` peut être
comme cela :

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__Note__ : Si le champ `main` n'est pas présent dans` package.json`, Electron
tentera de charger un fichier `index.js`.

Le fichier `main.js` devrait créer des fenêtres et gérer les événements du système.
Exemple :

```javascript
const {app, BrowserWindow} = require('electron')
const path = require('path')
const url = require('url')

// Gardez une référence globale de l'objet fenêtre, sinon, la fenêtre
// sera automatiquement fermée lorsque l'objet JavaScript est récupéré.
let win

function createWindow () {
  // Créer la fenêtre du navigateur.
  win = new BrowserWindow({width: 800, height: 600})

  // charger index.html de l'application.
  win.loadURL(url.format({
    pathname: path.join(__dirname, 'index.html'),
    protocol: 'file:',
    slashes: true
  }))

  // Ouvrir DevTools.
  win.webContents.openDevTools()

  // Émis lorsque la fenêtre est fermée.
  win.on('closed', () => {
    // Déréférencer l'objet fenêtre, habituellement vous stockez des fenêtres
    // dans un tableau si votre application prend en charge plusieurs fenêtres,
    // c'est l'heure où vous devez supprimer l'élément correspondant.
    win = null
  })
}

// Cette méthode sera appelée lorsque Electron aura terminé l'initialisation
// et est prét à créer des fenêtres de navigation. Certaines API ne peuvent
// être utilisées qu'après le lancement de cet événement.
app.on('ready', createWindow)

// Quittez lorsque toutes les fenêtres sont fermées.
app.on('window-all-closed', () => {
  // Sur macOS, il est fréquent que les applications et leur barre de menus
  // restent actives jusqu'à ce que l'utilisateur quitte explicitement avec Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  // Sur macOS, il est fréquent de recréer une fenêtre dans l'application lorsque
  // l'icône du dock est cliquée et qu'il n'y a pas d'autres fenêtres ouvertes.
  if (win === null) {
    createWindow()
  }
})

// Dans ce fichier, vous pouvez inclure le reste du code du processus principal
// spécifique de votre application. Vous pouvez également les mettres dans des
// fichiers distincts et les écrire ici.
```

Enfin, `index.html` est la page web que vous voulez afficher :

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    Nous utilisons le noeud <script>document.write(process.versions.node)</script>,
    Chrome <script>document.write(process.versions.chrome)</script>,
    et Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## Exécuter votre application

Une fois que vous avez créé vos fichiers `main.js`,` index.html` et `package.json`,
vous voudriez probablement essayer d'exécuter votre application localement pour la
tester et vous assurer qu'elle fonctionne comme prévu.

### `electron`

[`electron`](https://github.com/electron-userland/electron-prebuilt) est
un module `npm` qui contient des versions pré-compilées d'Electron.

Si vous l'avez installé globalement avec `npm`, vous n'en aurez pas besoin
dans le répertoire source de votre application :

```bash
electron .
```

Si vous l'avez installé localement :

#### macOS / Linux

```bash
$ ./node_modules/.bin/electron .
```

#### Windows

```bash
$ .\node_modules\.bin\electron .
```

### Executable d'Electron téléchargé manuellement

Si vous avez téléchargé Electron manuellement, vous pouvez également utiliser
binaire pour exécuter votre application directement.

#### Windows

```bash
$ .\electron\electron.exe your-app\
```

#### Linux

```bash
$ ./electron/electron your-app/
```

#### macOS

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` fait partie du package de libération de l'Electron, vous pouvez
télécharger à partir [ici](https://github.com/electron/electron/releases).

### Exécuter en tant que distribution

Une fois que vous avez terminé d'écrire votre application, vous pouvez
créer une distribution en suivant le guide [Distribuer une application](./application-distribution.md) puis exécuter l'application packagée.

### Essayez cet exemple

Clonez et exécutez le code dans ce didacticiel en utilisant le
[`electron/electron-quick-start`](https://github.com/electron/electron-quick-start).

**Note** : Exécuter cela nécessite [Git](https://git-scm.com) et [Node.js](https://nodejs.org/en/download/) (que comprend [npm](https://npmjs.org)) sur votre système.

```bash
# Clone the repository
$ git clone https://github.com/electron/electron-quick-start
# Go into the repository
$ cd electron-quick-start
# Install dependencies
$ npm install
# Run the app
$ npm start
```

Pour plus d'exemples app, consultez la section
[list of boilerplates](https://electron.atom.io/community/#boilerplates)
Créé par la communauté impressionnante d'électrons.

[share-data]: ../faq.md#how-to-share-data-between-web-pages

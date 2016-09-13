# Electron FAQ

## Quand est mise à jour la version de Chrome utilisée par Electron ?

La version de Chrome qu'utilise Electron est en général mise à jour une ou deux
semaines après la sortie d'une nouvelle version stable de Chrome.

Etant donné que nous n'utilisons que les versions stables de Chrome, si un fix
important est en beta ou en dev, nous l'intégrerons à la version que nous
utilisons.

## Quand est mise à jour la version de Node.js utilisée par Electron ?

Quand une nouvelle version de Node.js sort, nous attendons en général un mois
avant de mettre à jour celle que nous utilisons dans Electron. Ceci afin
d'éviter les bugs introduits par les nouvelles versions, ce qui arrive très
souvent.

Les nouvelles fonctionnalités de Node.js arrivant la plupart du temps via V8,
et Electron utilisant le V8 du navigateur Chrome, la nouvelle fonctionnalité
JavaScript de la nouvelle version de Node.js est bien souvent déjà dans
Electron.

## La fenêtre/barre d'état de mon application disparait après quelques minutes.

Cela se produit quand la variable qui est utilisée pour stocker la fenêtre/barre
d'état est libérée par le ramasse-miettes.

Nous vous recommandons de lire les articles suivants quand vous rencontrez le
problème :

* [Management de la Mémoire][memory-management] (Anglais)
* [Portée d'une Variable][variable-scope] (Anglais)

Si vous voulez corriger rapidement le problème, vous pouvez rendre les variables
globales en changeant votre code de ça :

```javascript
app.on('ready', function() {
  var tray = new Tray('/path/to/icon.png');
})
```

à ça :

```javascript
var tray = null;
app.on('ready', function() {
  tray = new Tray('/path/to/icon.png');
})
```

## Je n'arrive pas à utiliser jQuery/RequireJS/Meteor/AngularJS dans Electron.

A cause de l'intégration de Node.js dans Electron, certains mots-clés sont
insérés dans la DOM, comme `module`, `exports`, `require`. Ceci pose des
problèmes pour certaines bibliothèques qui utilisent les mêmes mots-clés.

Pour résoudre ce problème, vous pouvez désactiver l'intégration de node dans
Electron :

```javascript
// Dans le processus principal.
var mainWindow = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
});
```

Mais si vous voulez garder la possibilité d'utiliser Node.js et les APIs
Electron, vous devez renommer les mots-clés dans la page avant d'inclure
d'autres bibliothèques :

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

Lors de l'utilisation des modules d'Electron, vous pouvez avoir une erreur :

```
> require('electron').webFrame.setZoomFactor(1.0);
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

Ceci se produit quand vous avez le [module npm `electron`][electron-module]
d'installé, soit en local ou en global, ce qui a pour effet d'écraser les
modules de base d'Electron.

Vous vérifiez que vous utilisez les bons modules, vous pouvez afficher le
chemin du module `electron` :

```javascript
console.log(require.resolve('electron'));
```

et vérifier si il est de la forme :

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

S'il est de la forme `node_modules/electron/index.js`, vous devez supprimer le
module npm `electron`, ou le renommer.

```bash
npm uninstall electron
npm uninstall -g electron
```

Si vous utilisez le module de base mais que vous continuez d'avoir
l'erreur, ça vient probablement du fait que vous utilisez le module dans le
mauvais processus. Par exemple `electron.app` peut uniquement être utilisé
dans le processus principal, tandis que `electron.webFrame` est uniquement
disponible dans le processus d'affichage.

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron

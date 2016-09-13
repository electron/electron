Vérifiez que vous utilisez la bonne version de la documentation.
Le numéro de version devrait faire partie de l'URL de la page.
Si ce n'est pas le cas, vous utilisez probablement la documentation d'une
branche de développement qui peut contenir des changements API qui ne sont pas
compatibles avec votre version d'Electron. Si c'est le cas, vous pouvez changer
de version sur la liste [versions disponibles](http://electron.atom.io/docs/),
ou, si vous utilisez l'interface de GitHub, ouvrez la liste déroulante "Switch
branches/tags" afin de sélectionner le tag de votre version.

## FAQ

Avant de créer un ticket, vérifiez que votre problème n'a pas déjà sa réponse
dans la FAQ :

* [Electron FAQ](faq/electron-faq.md)

## Guides

* [Plateformes supportées](tutorial/supported-platforms.md)
* [Distribution de l'Application](tutorial/application-distribution.md)
* [Guide de Soumission Mac App Store](tutorial/mac-app-store-submission-guide.md)
* [Créer une archive](tutorial/application-packaging.md)
* [Utiliser Modules Natifs de Node](tutorial/using-native-node-modules.md)
* [Debugger Processus Principal](tutorial/debugging-main-process.md)
* [Utiliser Selenium et WebDriver](tutorial/using-selenium-and-webdriver.md)
* [Extension DevTools](tutorial/devtools-extension.md)
* [Utiliser le Plugin Pepper Flash](tutorial/using-pepper-flash-plugin.md)
* [Utiliser le Plugin Widevine CDM](tutorial/using-widevine-cdm-plugin.md)

## Tutoriels

* [Démarrage Rapide](tutorial/quick-start.md)
* [Intégration Environnement de Bureau](tutorial/desktop-environment-integration.md)
* [Détection des Evènements En ligne/Hors ligne](tutorial/online-offline-events.md)

## Références API

* [Synopsis](api/synopsis.md)
* [L'objet Process](api/process.md)
* [Commandes Chromes Supportées](api/chrome-command-line-switches.md)
* [Variables d'Environnement](api/environment-variables.md)

### Eléments DOM Personnalisés:

* [Objet `File`](api/file-object.md)
* [Tag `<webview>`](api/web-view-tag.md)
* [Fonction `window.open`](api/window-open.md)

### Modules pour le Processus Principal :

* [app](api/app.md)
* [autoUpdater](api/auto-updater.md)
* [BrowserWindow](api/browser-window.md)
* [contentTracing](api/content-tracing.md)
* [dialog](api/dialog.md)
* [globalShortcut](api/global-shortcut.md)
* [ipcMain](api/ipc-main.md)
* [Menu](api/menu.md)
* [MenuItem](api/menu-item.md)
* [powerMonitor](api/power-monitor.md)
* [powerSaveBlocker](api/power-save-blocker.md)
* [protocol](api/protocol.md)
* [session](api/session.md)
* [webContents](api/web-contents.md)
* [Tray](api/tray.md)

### Modules pour le Processus d'Affichage (Page Web) :

* [desktopCapturer](api/desktop-capturer.md)
* [ipcRenderer](api/ipc-renderer.md)
* [remote](api/remote.md)
* [webFrame](api/web-frame.md)

### Modules pour les deux Processus :

* [clipboard](api/clipboard.md)
* [crashReporter](api/crash-reporter.md)
* [nativeImage](api/native-image.md)
* [screen](api/screen.md)
* [shell](api/shell.md)

## Développement

* [Style de Code](development/coding-style.md)
* [Hiérarchie du Code Source](development/source-code-directory-structure.md)
* [Différences Techniques par rapport à NW.js (anciennement node-webkit)](development/atom-shell-vs-node-webkit.md)
* [Aperçu du Système de Build](development/build-system-overview.md)
* [Instructions de Build (macOS)](development/build-instructions-osx.md)
* [Instructions de Build (Windows)](development/build-instructions-windows.md)
* [Instructions de Build (Linux)](development/build-instructions-linux.md)
* [Installer un Serveur de Symbol dans le debugger](development/setting-up-symbol-server.md)

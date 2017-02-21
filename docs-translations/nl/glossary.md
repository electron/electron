# Woordenlijst

Deze pagina defineerd bepaalde terminologie die veel gebruikt wordt binnen Electron's ontwikkeling.

### ASAR

ASAR staat voor "Atom Shell Archive Format". Een [asar][asar] archief is een simpel `tar`-achtig formaat die bestanden samenvoegt in een enkel bestand. Electron kan er willekeurige bestanden uit lezen zonder het hele bestand uit te pakken.

De ASAR-indeling is in de eerste plaats gemaakt om de prestaties op Windows te verbeteren ... TODO

### Brightray

[Brightray][brightray] is een statische bibliotheek die[libchromiumcontent] gemakkelijker in applicaties te gebruiken maakt. Het werd speciaal gemaakt voor Electron, maar kan ook worden gebruikt om Chromium's renderer in applicaties die niet op Electron gebaseerd zijn in te schakelen.

Brightray is een laag-niveau afhankelijkheid van Electron, wat geen betrekking heeft op de meerderheid van Electron's gebruikers.

### DMG

Een Apple Disk Image is een pakket formaat gebruikt door macOS. DMG bestanden worden gebruikt voor het verspreiden van programma installeerders. [electron-builder] ondersteunt `dmg` als een bouw doelwit.

### IPC

IPC staat voor "Inter-Process Communicatie". Electron gebruikt IPC om geseralizeerde JSON berichten te sturen tussen het [hoofd] en het [renderer] proces.

### libchromiumcontent

Een enkele, gedeelde bibliotheek die de Chromium inhouds module en al zijn afhankelijkheden(e.g., Blink, [V8], etc.) bevat.

### main process

Het hoofd proces, vaak een bestand genoemd `main.js`, is het toeganspunt voor elke Electron applicatie. Het controleert de levensduur van de app, van open naar dicht. Het beheert ook de basis elementen zoals het menu, menubalk, etc. Het hoofd proces is verantwoordelijk voor het maken van elk nieuw renderer proces in de app. De volledige Node API is ingebouwd. 
Elke app's hoofd proces is opgegeven in het `main` attribuut in de `package.json`. Dit is hoe `electron .` weet welk bestand het moet uitvoeren bij het opstarten.

Zie ook: [process](#process), [renderer process](#renderer-process)

### MAS

Acroniem voor Apple's Mac App Store. Voor meer informatie over het indienen van uw app op de MAS, zie [Mac App Store Submission Guide].

### native modules

Native modules (ook wel [addons] in Node.js) zijn modules geschreven in C of C++ die kunnen ingeladen worden in Node.js of Electron met de `require()` functie, en gebruikt worden net alsof ze een gewone Node.js module zijn. Ze worden voornamelijk gebruikt om een een interface te voorzien tussen JavaScript die in Node.js loopt en C/C++. Native Node modules zijn ondersteund door Electron, maar aangezien Electron zeer waarschijnlijk een andere versie van V8 gebruikt dan uit de Node binaire geinstalleerd op uw systeem, moet u de locatie van Electron headers handmatig opgeven bij het bouwen van native modules.

Zie ook: [Using Native Node Modules].


[addons]: https://nodejs.org/api/addons.html
[asar]: https://github.com/electron/asar
[autoUpdater]: api/auto-updater.md
[brightray]: https://github.com/electron/brightray
[electron-builder]: https://github.com/electron-userland/electron-builder
[libchromiumcontent]: #libchromiumcontent
[Mac App Store Submission Guide]: tutorials/mac-app-store-submission-guide.md
[main]: #main-process
[renderer]: #renderer-process
[Using Native Node Modules]: tutorial/using-native-node-modules.md
[userland]: #userland
[V8]: #v8

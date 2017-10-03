# Woordenlijst

Deze pagina definieert bepaalde terminologie die veel gebruikt wordt binnen Electrons ontwikkeling.

### ASAR

ASAR staat voor "Atom Shell Archive Format". Een [asar][asar] archief is een simpel `tar`-achtig formaat dat bestanden samenvoegt in een enkel bestand. Electron kan er willekeurige bestanden uitlezen zonder het hele bestand uit te pakken.

De ASAR-indeling is in de eerste plaats gemaakt om de prestaties op Windows te verbeteren ... TODO

### Brightray

[Brightray][brightray] is een statische bibliotheek die [libchromiumcontent] gemakkelijker te gebruiken maakt in applicaties. Het werd speciaal gemaakt voor Electron, maar kan ook worden gebruikt om Chromiums renderer in applicaties, die niet op Electron gebaseerd zijn, in te schakelen.

Brightray is een laag-niveau afhankelijkheid van Electron, wat geen betrekking heeft op de meerderheid van Electrons gebruikers.

### DMG

Een Apple Disk Image is een pakket formaat gebruikt door macOS. DMG bestanden worden gebruikt voor het verspreiden van programma installers. [electron-builder] ondersteunt `dmg` als een build target.

### IPC

IPC staat voor "Inter-Process Communication". Electron gebruikt IPC om geserialiseerde JSON berichten te sturen tussen het [hoofd] en het [renderer] proces.

### libchromiumcontent

Een enkele, gedeelde bibliotheek die de Chromium content module en al zijn afhankelijkheden(e.g., Blink, [V8], etc.) bevat.

### main process

Het hoofdproces(main process), vaak een bestand genaamd `main.js`, is het toegangspunt voor elke Electron applicatie. Het controleert de levensduur van de app, van begin tot einde. Het beheert ook de basis elementen zoals het menu, de menubalk, etc. Het hoofdproces is verantwoordelijk voor het maken van elk nieuw renderer proces in de app. De volledige Node API is ingebouwd.
Elke app's hoofdproces is opgegeven in het `main` attribuut in de `package.json`. Dit is hoe `electron .` weet welk bestand het moet uitvoeren bij het opstarten.

Zie ook: [process](#process), [renderer process](#renderer-process)

### MAS

Acroniem voor Apple's Mac App Store. Voor meer informatie over het indienen van uw app op de MAS, zie [Mac App Store Submission Guide].

### native modules

Native modules (ook wel [addons] in Node.js) zijn modules geschreven in C of C++ die kunnen ingeladen worden in Node.js of Electron met de `require()` functie en kunnen gebruikt worden net alsof ze gewone Node.js module zijn. Ze worden voornamelijk gebruikt om een een interface te voorzien tussen JavaScript, die in Node.js loopt, en C/C++. Native Node modules worden ondersteund door Electron, maar aangezien Electron zeer waarschijnlijk een andere versie van V8 gebruikt dan de binaire Node geïnstalleerd op uw systeem, moet u de locatie van Electron headers handmatig opgeven bij het bouwen van native modules.

Zie ook: [Using Native Node Modules].

### NSIS

NSIS staat voor "Nullsoft Scriptable Install System", wat een script gestuurde installer authoring tool is voor Microsoft Windows. Het is vrijgegeven onder een combinatie van vrije software licenties en is een veel gebruikt alternatief voor commercieel gepatenteerde producten zoals InstallShield. [electron-builder] ondersteunt NSIS als een build target.

### process

Een proces is een instantie van een computerprogramma dat wordt uitgevoerd. Electron apps die gebruik maken van het [main] en één of meer [renderer] processen voeren eigenlijk meerdere programma's tegelijk uit.

In Node.js en Electron heeft elk lopend proces een `proces` object. Dit object is een globale die informatie en controle vooziet over het huidige proces. Als een globale is het altijd beschikbaar voor applicaties zonder gebruik van `require()`.

Zie ook: [main process](#main-process), [renderer process](#renderer-process)

### renderer process

Het renderer proces is een browservenster in uw app. In tegenstelling tot het hoofdproces, kunnen er meerdere renderer processen zijn en elk van deze is een apart proces. Ze kunnen ook verborgen zijn.

In normale browsers worden webpagina's meestal uitgevoerd in een sandbox-omgeving en hebben geen toegang tot native resources. Electron gebruikers hebben echter de bevoegdheid om gebruik te maken van de Node.js API in webpagina's die het toelaten om een lager niveau van besturingssysteem interacties te gebruiken.

Zie ook: [process](#process), [main process](#main-process)

### Squirrel

Squirrel is een open-source framework dat Electron apps in staat stelt om automatisch te updaten als nieuwe versies uitkomen. Kijk naar de [autoUpdater] API voor informatie over hoe te starten met Squirrel.

### userland

Userland of gebruikers-land is een term die zijn oorsprong heeft in de Unix gemeenschap, waar "userland" of "userspace" verwijst naar de programma's die uitgevoerd worden buiten de kernel. Recent is de term populair geworden in de Node en npm community om het verschil aan te geven tussen de beschikbare features in de "Node core" versus pakketten gepubliceerd naar het npm register door een veel grotere "gebruiker" gemeenschap.

Net zoals Node, is Electron gericht op het hebben van een kleine API die alle noodzakelijke functies voorziet voor het ontwikkelen van multi-platform desktop applicaties. Deze ontwerpfilosofie zorgt ervoor dat Electron een flexibele tool is zonder overdreven te beschrijven hoe het gebruikt moet worden. Userland zorgt ervoor dat gebruikers tools kunnen maken en delen die extra functionaliteit bieden bovenop wat beschikbaar is in de "core".

### V8

V8 is Google's open source JavaScript-engine. Het is geschreven in C++ en wordt gebruikt in Google Chrome. V8 kan standalone draaien, of kan worden gebruikt in een C++ applicatie.

### webview

`Webview` tags worden gebruikt om 'gast' content (zoals in externe webpagina's) in uw Electron app te verwerken. Ze lijken op `iframe`s, maar verschillen erin dat elke webview op een apart proces loopt. Het heeft niet dezelfde rechten als uw webpagina en alle interacties tussen uw app en ingebouwde inhoud zullen asynchroon zijn. Dit houdt uw app veilig voor de ingebouwde content.



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

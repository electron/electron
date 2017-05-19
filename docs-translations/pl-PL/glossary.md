# Słownik pojęć
Ta strona zawiera nieco terminologi, z którą możesz się spotkać podczas tworzenia aplikacji przy użyciu Electron.
### ASAR

ASAR to skrót pochodzący od Atom Shell Archive Format. Archiwum [asar][asar] jest prostym formatem podobnym do formatu `tar`, który zawiera łączy wiele plików w jeden. Electron czyta pliki wewnątrz bez rozpakowywania całego archiwum. 

Format ASAR został utworzony aby zwiększyć wydajność w systemie Windows.
### Brightray
Brightray to statyczna biblioteka czyni [libchromiumcontent] łatwiejszą do użycia w aplikacjach. Brightray to niskopoziomowa zależność Electron, która nie dotyczy większości użytkowników Electron. 

### DMG
Apple Disk Image to format pakowania używany przez macOS. Pliki DMG są powszechnie używane do dystrbuowania instalatorów aplikacji. [electron-builder] wspiera `dmg` jako cel budowania. 

### IPC
IPC oznacza Inter-Process Communication. Electron używa IPC do wysyłania serializowanych wiadomości JSON pomiędzy głównym i renderującym procesem.

### libchromiumcontent
Pojedyncza, udostępniana biblioteka, która zawiera moduł zawartości Chromium i wszystkie jego zależności (takie jak Blink, V8 i inne). 

### main process
Główny proces (ang. main process) powszechnie nazywany plikiem `main.js` to plik startowy każdej aplikacji Electron. Kontroluje funkcjonowanie aplikacji począwszy od otwarcia do zamknięcia. Zarządza także natywnymi elementami takimi jak Menu, Menu Bar, Dock, Tray i pozostałymi. Główny proces odpowiada za tworzenie nowych procesów renderujących w aplikacji. Pełne API Node jest wbudowane w ten proces.  

Każdy plik `main.js` jest wyszczególniony we właściwości w pliku `package.json`. Dzięki temu plikowi `electron` wie jaki plik powinien uruchomić.

Zobacz także: [Proces (process)](#process), [Proces renderujący (renderer process)](#renderer-process)

### MAS
Akronim dla Apple Mac App Store. Po więcej informacji jak dodać aplikację do MAS zajrzyj do [Przewodnik dodawania do Mac App Store]

### moduły natywne (native modules)
Moduły natywne (ang. native modules) zwane także dodatkami w Node.js to moduły napisane w C lub C++ które mogą być załadowane do Node.js lub Electron przy użyciu funkcji require(). Używane są jak zwykłe moduły Node.js. Służą głównie dostarczając interfejs pomiędzy uruchomionym JavaScript w Node.js i bibliotekami C/C++.

Zobacz także [Używanie natywnych modułów Node].

## NSIS
Nullsoft Scriptable Install System to operujący na skryptach instalator stworzony dla Microsoft Windows. Został wydany pod kombinacją wielu darmowych licencji i jest powszechnie używany jako alternatywa dla komercyjnych produktów takich jak InstallShield. [electron-builder] wspiera NSIS jako cel budowania.

### Proces (ang. process)
Proces to instancja programu komputerowego która jest aktualnie wykonywana. Aplikacje Electron wykorzystują główny proces [main] oraz jeden lub wiele procesów renderujących [renderer].

W Node.js i Electron każdy uruchomiony proces posiada obiekt `process`. Jest to globalny obiekt, który dostarcza informacje o aktualnym procesie i pozwala nim sterować. Jako obiekt globalny jest dostępny bez potrzeby stosowania require().  

Zobacz także: [Proces (process)](#process), [Proces renderujący (renderer process)](#renderer-process)

### Proces renderujący (ang. renderer process)
Zobacz także: [Proces (process)](#process), [Proces główny (main process)](#main-process)

Proces renderujący to okno przeglądarki w twojej aplikacji. W przeciwieństwie do procesu głównego może istnieć wiele procesów renderujących i każdy będzie uruchomiony oddzielnie. Mogą być one także ukrywane.

W normalnych przeglądarkach strony są zazwyczaj uruchamiane w oddzielonych środowiskach i nie mają dostępu do natywnych zasobów systemu. Użytkownicy Electron dzięki API Node.js mają możliwość używania i uruchamiania na stronach nisko poziomowych operacji systemowych.

### Squirrel

Squirrel is an open-source framework that enables Electron apps to update
automatically as new versions are released. See the [autoUpdater] API for
info about getting started with Squirrel.

Squirrel to framework na licencji open source, który pozwala na automatyczne aktualizacje aplikacji Electron gdy zostanie wypuszczona nowa wersja. Zobacz [autoUpdater] API po więcej informacji dotyczących Squirrel.

### userland

This term originated in the Unix community, where "userland" or "userspace"
referred to programs that run outside of the operating system kernel. More
recently, the term has been popularized in the Node and npm community to
distinguish between the features available in "Node core" versus packages
published to the npm registry by the much larger "user" community.

Like Node, Electron is focused on having a small set of APIs that provide
all the necessary primitives for developing multi-platform desktop applications.
This design philosophy allows Electron to remain a flexible tool without being
overly prescriptive about how it should be used. Userland enables users to
create and share tools that provide additional functionality on top of what is
available in "core".

### V8

V8 is Google's open source JavaScript engine. It is written in C++ and is
used in Google Chrome. V8 can run
standalone, or can be embedded into any C++ application.

### webview

`webview` tags are used to embed 'guest' content (such as external web pages) in
your Electron app. They are similar to `iframe`s, but differ in that each
webview runs in a separate process. It doesn't have the same
permissions as your web page and all interactions between your app and
embedded content will be asynchronous. This keeps your app safe from the
embedded content.

[addons]: https://nodejs.org/api/addons.html
[asar]: https://github.com/electron/asar
[autoUpdater]: api/auto-updater.md
[electron-builder]: https://github.com/electron-userland/electron-builder
[libchromiumcontent]: #libchromiumcontent
[Przewodnik dodawania do Mac App Store]: tutorials/mac-app-store-submission-guide.md
[main]: #main-process
[renderer]: #renderer-process
[Używanie natywnych modułów Node]: tutorial/using-native-node-modules.md
[userland]: #userland
[V8]: #v8

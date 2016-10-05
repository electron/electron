# Bijdragen aan Electron

:memo: Beschikbare vertalingen: [Koreaans](https://github.com/electron/electron/tree/master/docs-translations/ko-KR/project/CONTRIBUTING.md) | [Versimpeld Chinees](https://github.com/electron/electron/tree/master/docs-translations/zh-CN/project/CONTRIBUTING.md) | [Braziliaans Portugees](https://github.com/electron/electron/tree/master/docs-translations/pt-BR/project/CONTRIBUTING.md)

:+1::tada: Ten eerste, bedankt om de tijd te nemen om bij te dragen! :tada::+1:

Dit project volgt de  [gedragscode](CODE_OF_CONDUCT.md) van bijdragers.
Door deel te nemen, wordt je verwacht je aan deze code te houden. Onaanvaardbaar gedrag moet gerapporteerd worden bij atom@github.com.

Dit is een set van richtlijnen om bij te dragen aan Electron.
Dit zijn slechts richtijnen, geen regels, gebruik je gezond verstand voe je vrij om wijzigingen aan dit document voor te stellen in een pull request.

## Issues indienen

* Je kan [hier](https://github.com/electron/electron/issues/new) een issue indienen,
maar lees eerst onderstaande suggesties en voeg zo veel mogelijk details toe bij je indiening. Indien mogelijk, voeg dan toe:
  * De versie van Electron die je gebruikt
  * Het besturingssysteem dat je gebruikt
  * Indien mogelijk, wat je aan het doen was toen het probleem zich voor deed en wat je verwachtte dat zou gebeuren
* Andere dingen die kunnen helpen bij het oplossen van je probleem:
  * Schermafbeeldingen en geanimeerde GIFs
  * Foutmeldingen die voorkomen in je terminal, ontwikkeltools of als melding
  * Doe een [vluchtig onderzoek](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)
  om te zien of een gelijkaardig probleem al is gemeld

## Pull Requests Indienen

* Voeg screenshots en geanimeerde GIFs toe in je pull request wanneer mogelijk.
* Volg de JavaScript, C++ en Python [codeer-stijl gedefinieerd in de documentatie](/docs/development/coding-style.md).
* Schrijf documentatie in in [Markdown](https://daringfireball.net/projects/markdown).
  Zie de [Documentatie Stijlgids](/docs/styleguide.md).
* Gebruik korte commit-berichten in de tegenwoordige tijd. Zie [Commit Bericht Stijlgids](#git-commit-messages).

## Stijlgidsen

### Code

* Beëindig bestanden met een newline.
* Requires in de volgende volgorde:
  * Ingebouwde Node Modules (zoals `path`)
  * Ingebouwde Electron Modules (zoals `ipc`, `app`)
  * Lokale Modules (met relatieve paden)
* Klasse-properties moeten in volgende volgorde:
  * Klasse-methodes en properties (methodes starten met `@`)
  * Instance-methodes en properties
* Vermijd platform-afhankelijke code:
  * Gebruik `path.join()` om filenames te concateneren.
  * Gebruik `os.tmpdir()` in plaats van `/tmp` voor de tijdelijke directory.
* Gebruik een gewone `return` wanneer je expliciet uit een functie returned.
  * Geen `return null`, `return undefined`, `null`, of `undefined`

### Git Commit Berichten

* Gebruik tegenwoordige tijd ("Add feature" niet "Added feature")
* Gebruik gebiedende wijs ("Move cursor to..." niet "Moves cursor to...")
* De eerste lijn mag maximum 72 karakters lang zijn
* Refereer uitvoerig naar issues en pull requests
* Wanneer je enkel documentatie wijzigt, voeg dan `[ci skip]` toe aan het commit-bericht
* Wanneer toepasbaar, begin je commit-bericht met volgende emoji:
  * :art: `:art:` als het formaat/de structuur van de code wordt verbeterd
  * :racehorse: `:racehorse:` als de performantie van de code wordt verbeterd
  * :non-potable_water: `:non-potable_water:` als memory leaks worden opgelost
  * :memo: `:memo:` als documentie is geschreven
  * :penguin: `:penguin:` als er iets Linux-gerelateerd is opgelost
  * :apple: `:apple:` als er iets macOS-gerelateerd is opgelost
  * :checkered_flag: `:checkered_flag:` als er iets Windows-gerelateerd is opgelost
  * :bug: `:bug:` als een bug is opgelost
  * :fire: `:fire:` als code of bestanden worden verwijderd
  * :green_heart: `:green_heart:` als de CI build wordt gefixt
  * :white_check_mark: `:white_check_mark:` als er tests worden toegevoegd
  * :lock: `:lock:` als er iets met security wordt gedaan
  * :arrow_up: `:arrow_up:` als dependencies worden geüpgraded
  * :arrow_down: `:arrow_down:` als dependencies worden gedownpgraded
  * :shirt: `:shirt:` als linter waarschuwingen worden opgelost

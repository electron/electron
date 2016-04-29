# Règles de style pour la documentation d'Electron

Choisissez la section appropriée : [lire la documentation d'Electron](#reading-electron-documentation)
ou [écrire de la documentation pour Electron](#writing-electron-documentation).

## Ecrire de la documentation pour Electron

La documentation d'Electron a été écrite en suivant les règles ci-dessous :

- Maximum un titre `h1` par page.
- Utilisation de `bash` au lieu de `cmd` dans les blocs de code (à cause de la
  coloration syntaxique).
- Les titres `h1` devraient reprendre le nom de l'objet (i.e. `browser-window` →
  `BrowserWindow`).
  - Cependant, les traits d'union sont acceptés pour les noms de fichier.
- Pas de titre directement après un autre, ajoutez au minimum une ligne de
  description entre les deux.
- Les entêtes des méthodes sont entre accents graves (backquotes) `code`.
- Les entêtes des évènements sont entre des apostrophes 'quotation'.
- Les listes ne doivent pas dépasser 2 niveaux (à cause du formattage du
  markdown).
- Ajouter des titres de section: Evènements, Méthodes de classe, et Méthodes
  d'instance.
- Utiliser 'will' au lieu de 'would' lors de la description du retour.
- Les évènements et méthodes sont des titres `h3`.
- Les arguments optionnels sont notés `function (required[, optional])`.
- Les arguments optionnels sont indiqués quand appelés dans la liste.
- La longueur des lignes ne dépasse pas 80 caractères.
- Les méthodes spécifiques à une plateforme sont notées en italique.
 - ```### `method(foo, bar)` _OS X_```
- Préférer 'in the ___ process' au lieu de 'on'

### Traductions de la Documentation

Les traductions de la documentation d'Electron sont dans le dossier
`docs-translations`.

Pour ajouter une nouvelle langue (ou commencer) :

- Créer un sous-dossier avec comme nom le code langage.
- A l'intérieur de ce dossier, dupliquer le dossier `docs`, en gardant le même
  nom de dossiers et de fichiers.
- Traduire les fichiers.
- Mettre à jour le `README.md` à l'intérieur du dossier de langue en mettant les
  liens vers les fichiers traduits.
- Ajouter un lien vers le nouveau dossier de langue dans le [README](https://github.com/electron/electron#documentation-translations)
  principal d'Electron.

## Lire la documentation d'Electron

Quelques indications pour comprendre la syntaxe de la documentation d'Electron.

### Méthodes

Un exemple de la documentation d'une [méthode](https://developer.mozilla.org/en-US/docs/Glossary/Method)
(Anglais)

---

`methodName(required[, optional]))`

* `require` String (**required**)
* `optional` Integer

---

Le nom de la méthode est suivi des arguments de celle-ci. Les arguments
optionnels sont notés entre crochets, avec une virgule si ceux-ci suivent un
autre argument.

En-dessous de la méthode, chaque argument est détaillé avec son type.
Celui-ci peut être un type générique :
[`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String),
[`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number),
[`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object),
[`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
ou un type personnalisé comme le [`webContent`](api/web-content.md) d'Electron.

### Evènements

Un exemple d'une documentation d'un [évènement](https://developer.mozilla.org/en-US/docs/Web/API/Event)
(Anglais)
---

Event: 'wake-up'

Returns:

* `time` String

---

L'évènement est une chaine utilisée après un listener `.on`. Si il retourne une
valeur, elle est écrite en dessous ainsi que son type. Si vous voulez écouter et
répondre à l'évènement wake-up, ça donne quelque chose comme :

```javascript
Alarm.on('wake-up', function(time) {
  console.log(time)
})
```

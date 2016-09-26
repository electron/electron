# Electron Documentation Styleguide

These are the guidelines for writing Electron documentation.

## Titles

* Each page must have a single `#`-level title at the top.
* Chapters in the same page must have `##`-level titles.
* Sub-chapters need to increase the number of `#` in the title according to
  their nesting depth.
* All words in the page's title must be capitalized, except for conjunctions
  like "of" and "and" .
* Only the first word of a chapter title must be capitalized.

Using `Quick Start` as example:

```markdown
# Quick Start

...

## Main process

...

## Renderer process

...

## Run your app

...

### Run as a distribution

...

### Manually downloaded Electron binary

...
```

For API references, there are exceptions to this rule.

## Markdown rules

* Use `bash` instead of `cmd` in code blocks (due to the syntax highlighter).
* Lines should be wrapped at 80 columns.
* No nesting lists more than 2 levels (due to the markdown renderer).
* All `js` and `javascript` code blocks are linted with
[standard-markdown](http://npm.im/standard-markdown).

## Picking words

* Use "will" over "would" when describing outcomes.
* Prefer "in the ___ process" over "on".

## API references

The following rules only apply to the documentation of APIs.

### Page title

Each page must use the actual object name returned by `require('electron')`
as the title, such as `BrowserWindow`, `autoUpdater`, and `session`.

Under the page tile must be a one-line description starting with `>`.

Using `session` as example:

```markdown
# session

> Manage browser sessions, cookies, cache, proxy settings, etc.
```

### Module methods and events

For modules that are not classes, their methods and events must be listed under
the `## Methods` and `## Events` chapters.

Using `autoUpdater` as an example:

```markdown
# autoUpdater

## Events

### Event: 'error'

## Methods

### `autoUpdater.setFeedURL(url[, requestHeaders])`
```

### Classes

* API classes or classes that are part of modules must be listed under a
  `## Class: TheClassName` chapter.
* One page can have multiple classes.
* Constructors must be listed with `###`-level titles.
* [Static Methods](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes/static) must be listed under a `### Static Methods` chapter.
* [Instance Methods](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes#Prototype_methods) must be listed under an `### Instance Methods` chapter.
* Instance Events must be listed under an `### Instance Events` chapter.
* Instance Properties must be listed under an `### Instance Properties` chapter.

Using the `Session` and `Cookies` classes as an example:

```markdown
# session

## Methods

### session.fromPartition(partition)

## Properties

### session.defaultSession

## Class: Session

### Instance Events

#### Event: 'will-download'

### Instance Methods

#### `ses.getCacheSize(callback)`

### Instance Properties

#### `ses.cookies`

## Class: Cookies

### Instance Methods

#### `cookies.get(filter, callback)`
```

### Methods

The methods chapter must be in the following form:

```markdown
### `objectName.methodName(required[, optional]))`

* `required` String
* `optional` Integer (optional)

...
```

The title can be `###` or `####`-levels depending on whether it is a method of
a module or a class.

For modules, the `objectName` is the module's name. For classes, it must be the
name of the instance of the class, and must not be the same as the module's
name.

For example, the methods of the `Session` class under the `session` module must
use `ses` as the `objectName`.

The optional arguments are notated by square brackets `[]` surrounding the optional argument
as well as the comma required if this optional argument follows another
argument:

```
required[, optional]
```

Below the method is more detailed information on each of the arguments. The type
of argument is notated by either the common types:

* [`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)
* [`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number)
* [`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)
* [`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
* [`Boolean`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)
* Or a custom type like Electron's [`WebContent`](api/web-contents.md)

If an argument or a method is unique to certain platforms, those platforms are
denoted using a space-delimited italicized list following the datatype. Values
can be `macOS`, `Windows`, or `Linux`.

```markdown
* `animate` Boolean (optional) _macOS_ _Windows_
```

`Array` type arguments must specify what elements the array may include in
the description below.

The description for `Function` type arguments should make it clear how it may be
called and list the types of the parameters that will be passed to it.

### Events

The events chapter must be in following form:

```markdown
### Event: 'wake-up'

Returns:

* `time` String

...
```

The title can be `###` or `####`-levels depending on whether it is an event of
a module or a class.

The arguments of an event follow the same rules as methods.

### Properties

The properties chapter must be in following form:

```markdown
### session.defaultSession

...
```

The title can be `###` or `####`-levels depending on whether it is a property of
a module or a class.

## Documentation Translations

Translations of the Electron docs are located within the `docs-translations`
directory.

To add another set (or partial set):

* Create a subdirectory named by language abbreviation.
* Translate the files.
* Update the `README.md` within your language directory to link to the files
  you have translated.
* Add a link to your translation directory on the main Electron
  [README](https://github.com/electron/electron#documentation-translations).

Note that the files under `docs-translations` must only include the translated
ones, the original English files should not be copied there.

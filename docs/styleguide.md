# Electron Documentation Styleguide

These are the ways that we construct the Electron documentation.

## Titles

Each page has only one `#`-level title on the top, following chapters in the
same page must have `##`-level titles, and sub-chapters need to increase the
number of `#` in title according to its nesting depth.

For the page's title, all words must be capitalized, for other chapters, only
the first word has to be capitalized.

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

* Use `bash` instead of `cmd` in code blocks (because of syntax highlighter).
* Line length is 80-column wrapped.
* No nesting lists more than 2 levels (unfortunately because of markdown
  renderer).

## Picking words

* Use "will" over "would" when describing outcomes.
* Prefer "in the ___ process" over "on".

## API references

Following rules only apply to documentations of APIs.

### Page title

Each page must use the actual object name returned by `require('electron')`
as name, like `BrowserWindow`, `autoUpdater`, `session`.

Under the page tile must be a one-line description starting with `>`.

Using `session` as example:

```markdown
# session

> Manage browser sessions, cookies, cache, proxy settings, etc.

```

### Module methods and events

For modules that are not classes, their methods and events must be listed under
`## Methods` and `## Events` chapters.

Using `autoUpdater` as example:

```markdown
# autoUpdater

## Events

### Event: 'error'

## Methods

### `autoUpdater.setFeedURL(url[, requestHeaders])`
```

### Classes

For APIs that exists as classes, or for classes that are parts of modules, they
must listed under `## Class: TheClassName` chapter. One page can have multiple
classes.

If the class has constructors, they must be listed with `###`-level titles.

The methods, events and properties of a class must be listed under
`### Instance Methods`, `### Instance Events` and `Instance Properties`
chapters.

Using `Session`, `Cookies` classes as example:

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

The chapter of method must be in the following form:

```markdown
### `objectName.methodName(required[, optional]))`

* `required` String
* `optional` Integer (optional)

...

```

The title can be `###` or `####`-levels depending on whether it is a method of
module or class.

For modules, the `objectName` is the module's name, for classes, it must be the
name of instance of the class, and must not be same with the module's name.

For example, the methods of `Session` class under `session` module must use
`ses` as `objectName`.

The optional arguments are notated by brackets surrounding the optional argument
as well as the comma required if this optional argument follows another
argument.

Below the method is more detailed information on each of the arguments. The type
of argument is notated by either the common types:
[`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String),
[`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number),
[`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object),
[`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
or a custom type like Electron's [`WebContent`](api/web-content.md).

If an argument or a method is unique to certain platforms, those platforms are
denoted using a space-delimited italicized list following the datatype. Values
can be `macOS`, `Windows`, or `Linux`.

``` markdown
* `animate` Boolean (optional) _macOS_ _Windows_
```

For argument with `Array` type, it must be classified what elements the array
may include in the description below.

For argument with `Function` type, the description should make it clear how it
may be called and list the types of the arguments that passed to it.

### Events

The chapter of event must be in following form:

```markdown
### Event: 'wake-up'

Returns:

* `time` String

...

```

The title can be `###` or `####`-levels depending on whether it is an event of
module or class.

The arguments of an event have the same rules with methods.

### Properties

The chapter of property must be in following form:

```markdown
### session.defaultSession

...

```

The title can be `###` or `####`-levels depending on whether it is a property of
module or class.

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

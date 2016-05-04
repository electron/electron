# Electron Documentation Styleguide

Find the appropriate section for your task: [reading Electron documentation](#reading-electron-documentation)
or [writing Electron documentation](#writing-electron-documentation).

## Writing Electron Documentation

These are the ways that we construct the Electron documentation.

- Maximum one `h1` title per page.
- Use `bash` instead of `cmd` in code blocks (because of syntax highlighter).
- Doc `h1` titles should match object name (i.e. `browser-window` →
  `BrowserWindow`).
  - Hyphen separated filenames, however, are fine.
- No headers following headers, add at least a one-sentence description.
- Methods headers are wrapped in `code` ticks.
- Event headers are wrapped in single 'quotation' marks.
- No nesting lists more than 2 levels (unfortunately because of markdown
  renderer).
- Add section titles: Events, Class Methods and Instance Methods.
- Use 'will' over 'would' when describing outcomes.
- Events and methods are `h3` headers.
- Optional arguments written as `function (required[, optional])`.
- Optional arguments are denoted when called out in list.
- Line length is 80-column wrapped.
- Platform specific methods are noted in italics following method header.
 - ```### `method(foo, bar)` _OS X_```
- Prefer 'in the ___ process' over 'on'

### Documentation Translations

Translations of the Electron docs are located within the `docs-translations`
directory.

To add another set (or partial set):

- Create a subdirectory named by language abbreviation.
- Within that subdirectory, duplicate the `docs` directory, keeping the
  names of directories and files same.
- Translate the files.
- Update the `README.md` within your language directory to link to the files
  you have translated.
- Add a link to your translation directory on the main Electron [README](https://github.com/electron/electron#documentation-translations).

## Reading Electron Documentation

Here are some tips for understanding Electron documentation syntax.

### Methods

An example of [method](https://developer.mozilla.org/en-US/docs/Glossary/Method)
documentation:

---

`methodName(required[, optional]))`

* `require` String (**required**)
* `optional` Integer

---

The method name is followed by the arguments it takes. Optional arguments are
notated by brackets surrounding the optional argument as well as the comma
required if this optional argument follows another argument.

Below the method is more detailed information on each of the arguments. The type
of argument is notated by either the common types:
[`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String),
[`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number),
[`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object),
[`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
or a custom type like Electron's [`webContent`](api/web-content.md).

### Events

An example of [event](https://developer.mozilla.org/en-US/docs/Web/API/Event)
documentation:

---

Event: 'wake-up'

Returns:

* `time` String

---

The event is a string that is used after a `.on` listener method. If it returns
a value it and its type is noted below. If you were to listen and respond to
this event it might look something like this:

```javascript
Alarm.on('wake-up', (time) => {
  console.log(time)
})
```

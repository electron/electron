# Electron Dokümantasyonu Stil Rehberi

Size uygun bölümü bulun: [Electron Dokümantasyonunu okumak](#reading-electron-documentation)
ya da [Electron Dokümantasyonunu yazmak](#writing-electron-documentation).

## Electron Dokümantasyonunu Yazmak

Electron Dokümantasyonunu geliştirmek için aşağıdaki yöntemleri takip edin.

- Her sayfada en fazla bir tane `h1` etiketi olmalıdır.
- Kod bloklarında `cmd` yerine `bash` kullanın.(syntax highlighter için).
- `h1` Başlığı nesne ismiyle eşleşmeli (ör. `browser-window` →
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

### Dokümantasyon Çevirisi

Electron Dokümantasyonunun çevirileri `docs-translations` klasörü içerisindedir.

To add another set (or partial set):

- Create a subdirectory named by language abbreviation.
- Within that subdirectory, duplicate the `docs` directory, keeping the
  names of directories and files same.
- Translate the files.
- Update the `README.md` within your language directory to link to the files
  you have translated.
- Add a link to your translation directory on the main Electron [README](https://github.com/electron/electron#documentation-translations).

## Electron Dokümantasyonunu Okumak

Electron Dokümantasyon sözdizimini(syntax) anlayabileceğiniz bir kaç ipucu:

### Metodlar

[Method](https://developer.mozilla.org/en-US/docs/Glossary/Method) dokümantasyonunun bir örneği:

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
or a custom type like Electron's [`webContent`](https://github.com/electron/electron/tree/master/docs/api/web-content.md).

### Events

[event](https://developer.mozilla.org/en-US/docs/Web/API/Event) Dokümantasyonunun bir örneği:

---

Event: 'wake-up'

Returns:

* `time` String

---

The event is a string that is used after a `.on` listener method. If it returns
a value it and its type is noted below. If you were to listen and respond to
this event it might look something like this:

```javascript
Alarm.on('wake-up', function(time) {
  console.log(time)
})
```

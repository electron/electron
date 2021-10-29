---
title: "SpellChecker"
description: "Learn how to use the built-in spellchecker, set languages, etc."
slug: spellchecker
hide_title: false
---

# SpellChecker

Electron has built-in support for Chromium's spellchecker since Electron 8.  On Windows and Linux this is powered by Hunspell dictionaries, and on macOS it makes use of the native spellchecker APIs.

## How to enable the spellchecker?

For Electron 9 and higher the spellchecker is enabled by default.  For Electron 8 you need to enable it in `webPreferences`.

```js
const myWindow = new BrowserWindow({
  webPreferences: {
    spellcheck: true
  }
})
```

## How to set the languages the spellchecker uses?

On macOS as we use the native APIs there is no way to set the language that the spellchecker uses. By default on macOS the native spellchecker will automatically detect the language being used for you.

For Windows and Linux there are a few Electron APIs you should use to set the languages for the spellchecker.

```js
// Sets the spellchecker to check English US and French
myWindow.session.setSpellCheckerLanguages(['en-US', 'fr'])

// An array of all available language codes
const possibleLanguages = myWindow.session.availableSpellCheckerLanguages
```

By default the spellchecker will enable the language matching the current OS locale.

## How do I put the results of the spellchecker in my context menu?

All the required information to generate a context menu is provided in the [`context-menu`](../api/web-contents.md#event-context-menu) event on each `webContents` instance.  A small example
of how to make a context menu with this information is provided below.

```js
const { Menu, MenuItem } = require('electron')

myWindow.webContents.on('context-menu', (event, params) => {
  const menu = new Menu()

  // Add each spelling suggestion
  for (const suggestion of params.dictionarySuggestions) {
    menu.append(new MenuItem({
      label: suggestion,
      click: () => mainWindow.webContents.replaceMisspelling(suggestion)
    }))
  }

  // Allow users to add the misspelled word to the dictionary
  if (params.misspelledWord) {
    menu.append(
      new MenuItem({
        label: 'Add to dictionary',
        click: () => mainWindow.webContents.session.addWordToSpellCheckerDictionary(params.misspelledWord)
      })
    )
  }

  menu.popup()
})
```

## Does the spellchecker use any Google services?

Although the spellchecker itself does not send any typings, words or user input to Google services the hunspell dictionary files are downloaded from a Google CDN by default.  If you want to avoid this you can provide an alternative URL to download the dictionaries from.

```js
myWindow.session.setSpellCheckerDictionaryDownloadURL('https://example.com/dictionaries/')
```

Check out the docs for [`session.setSpellCheckerDictionaryDownloadURL`](../api/session.md#sessetspellcheckerdictionarydownloadurlurl) for more information on where to get the dictionary files from and how you need to host them.

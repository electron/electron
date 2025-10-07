## Class: NavigationHistory

> Manage a list of navigation entries, representing the user's browsing history within the application.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Each [NavigationEntry](./structures/navigation-entry.md) corresponds to a specific visited page.
The indexing system follows a sequential order, where the entry for the earliest visited
page is at index 0 and the entry for the most recent visited page is at index N.

Some APIs in this class also accept an _offset_, which is an integer representing the relative
position of an index from the current entry according to the above indexing system (i.e. an offset
value of `1` would represent going forward in history by one page).

Maintaining this ordered list of navigation entries enables seamless navigation both backward and
forward through the user's browsing history.

### Instance Methods

#### `navigationHistory.canGoBack()`

Returns `boolean` - Whether the browser can go back to previous web page.

#### `navigationHistory.canGoForward()`

Returns `boolean` - Whether the browser can go forward to next web page.

#### `navigationHistory.canGoToOffset(offset)`

* `offset` Integer

Returns `boolean` - Whether the web page can go to the specified relative `offset` from the current entry.

#### `navigationHistory.clear()`

Clears the navigation history.

#### `navigationHistory.getActiveIndex()`

Returns `Integer` - The index of the current page, from which we would go back/forward or reload.

#### `navigationHistory.getEntryAtIndex(index)`

* `index` Integer

Returns [`NavigationEntry`](structures/navigation-entry.md) - Navigation entry at the given index.

If index is out of bounds (greater than history length or less than 0), null will be returned.

#### `navigationHistory.goBack()`

Makes the browser go back a web page.

#### `navigationHistory.goForward()`

Makes the browser go forward a web page.

#### `navigationHistory.goToIndex(index)`

* `index` Integer

Navigates browser to the specified absolute web page index.

#### `navigationHistory.goToOffset(offset)`

* `offset` Integer

Navigates to the specified relative offset from the current entry.

#### `navigationHistory.length()`

Returns `Integer` - History length.

#### `navigationHistory.removeEntryAtIndex(index)`

* `index` Integer

Removes the navigation entry at the given index. Can't remove entry at the "current active index".

Returns `boolean` - Whether the navigation entry was removed from the webContents history.

#### `navigationHistory.getAllEntries()`

Returns [`NavigationEntry[]`](structures/navigation-entry.md) - WebContents complete history.

#### `navigationHistory.restore(options)`

Restores navigation history and loads the given entry in the in stack. Will make a best effort
to restore not just the navigation stack but also the state of the individual pages - for instance
including HTML form values or the scroll position. It's recommended to call this API before any
navigation entries are created, so ideally before you call `loadURL()` or `loadFile()` on the
`webContents` object.

This API allows you to create common flows that aim to restore, recreate, or clone other webContents.

* `options` Object
  * `entries` [NavigationEntry[]](structures/navigation-entry.md) - Result of a prior `getAllEntries()` call
  * `index` Integer (optional) - Index of the stack that should be loaded. If you set it to `0`, the webContents will load the first (oldest) entry. If you leave it undefined, Electron will automatically load the last (newest) entry.

Returns `Promise<void>` - the promise will resolve when the page has finished loading the selected navigation entry
(see [`did-finish-load`](web-contents.md#event-did-finish-load)), and rejects
if the page fails to load (see
[`did-fail-load`](web-contents.md#event-did-fail-load)). A noop rejection handler is already attached, which avoids unhandled rejection errors.

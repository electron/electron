## Class: NavigationHistory

> Handles session history, maintaining a list of navigation entries for backward and forward navigation.
Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### Instance Methods

#### `navigationHistory.getActiveIndex()`

Returns `Integer` - The index from which we would go back/forward or reload.

#### `navigationHistory.getEntryAtIndex(index)`

* `index` Integer

Returns `Object`:

* `url` string - The url of the navigation entry at the given index.
* `title` string - The app title set by the page to be displayed on the tab.

If index is invalid (greater than history length or less than 0), null will be returned.

#### `navigationHistory.length()`

Returns `Integer` - History length.
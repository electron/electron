## Class: NavigationHistory

> Manage a list of navigation entries, representing the user's browsing history within the application.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Each navigation entry corresponds to a specific page. The indexing system follows a sequential order, where the first available navigation entry is at index 0, representing the earliest visited page, and the latest navigation entry is at index N, representing the most recent page. Maintaining this ordered list of navigation entries enables seamless navigation both backward and forward through the user's browsing history.

### Instance Methods

#### `navigationHistory.getActiveIndex()`

Returns `Integer` - The index of the current page, from which we would go back/forward or reload.

#### `navigationHistory.getEntryAtIndex(index)`

* `index` Integer

Returns `Object`:

* `url` string - The URL of the navigation entry at the given index.
* `title` string - The page title of the navigation entry at the given index.

If index is out of bounds (greater than history length or less than 0), null will be returned.

#### `navigationHistory.length()`

Returns `Integer` - History length.

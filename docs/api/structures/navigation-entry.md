# NavigationEntry Object

* `url` string
* `title` string
* `pageState` string (optional) - A base64 encoded data string containing Chromium page state
  including information like the current scroll position or form values. It is committed by
  Chromium before a navigation event and on a regular interval.
* `shouldSkipOnBackForwardUI` boolean (optional) - Whether Chromium's
  [history manipulation intervention](https://chromestatus.com/feature/5585007072116736) will skip
  this entry on back/forward navigations because the page added it to the history without the user
  ever interacting with it (for example a client-side redirect). Skipped entries are not committed
  by `navigationHistory.goBack()` and `navigationHistory.goForward()`, but can still be reached
  with `navigationHistory.goToIndex(index)`. Always present on entries returned by Electron; it is
  ignored when passed to `navigationHistory.restore()`.

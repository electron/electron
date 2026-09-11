# ApiBridgeCaller Object

* `frame` [WebFrameMain](../web-frame-main.md) - The frame the call came from.
  This is the frame the API was given to, also when a same-origin iframe calls
  it through `parent.navigator.electron`.
* `origin` string - The origin of the document that made the call, such as
  `https://example.com`. Useful when an API allows more than one origin.
* `signal` AbortSignal - Aborts when the document that made the call goes
  away, or when the API is revoked from it. Use it to stop long work early.

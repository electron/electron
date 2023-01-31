# NavigationEvent Object extends `GlobalEvent`

* `preventDefault` VoidFunction
* `url` string - The URL the frame is navigating to.
* `isSameDocument` boolean - Whether the navigation happened without changing
  document. Examples of same document navigations are reference fragment
  navigations, pushState/replaceState, and same page history navigation.
* `isMainFrame` boolean - True if the navigation is taking place in a main frame.
* `frame` WebFrameMain - The frame to be navigated.
* `initiator` WebFrameMain (optional) - The frame which initiated the
  navigation, which can be a parent frame (e.g. via `window.open` with a
  frame's name), or null if the navigation was not initiated by a frame. This
  can also be null if the initiating frame was deleted before the event was
  emitted.

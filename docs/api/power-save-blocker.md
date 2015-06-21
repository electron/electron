# power-save-blocker

The `power-save-blocker` module is used to block the system from entering
low-power(sleep) mode.

An example is:

```javascript
var powerSaveBlocker = require('power-save-blocker');

powerSaveBlocker.start(powerSaveBlocker.PREVENT_DISPLAY_SLEEP);
console.log(powerSaveBlocker.IsStarted());
```

## powerSaveBlocker.start(type)

* type - Power save blocker type
  * powerSaveBlocker.PREVENT_APP_SUSPENSION - Prevent the application from being
    suspended. On some platforms, apps may be suspended when they are not visible
    to the user.  This type of block requests that the app continue to run in that
    case,and on all platforms prevents the system from sleeping.
    Example use cases: downloading a file, playing audio.
  * powerSaveBlocker.PREVENT_DISPLAY_SLEEP - Prevent the display from going to sleep.
    This also has the side effect of preventing the system from sleeping, but
    does not necessarily prevent the app from being suspended on some platforms
    if the user hides it.
    Example use case: playing video.

Starts the power save blocker preventing the system entering lower-power mode.

## powerSaveBlocker.isStarted()

Returns whether the `powerSaveBlocker` starts.

## powerSaveBlocker.stop()

Stops blocking the system from entering low-power mode.

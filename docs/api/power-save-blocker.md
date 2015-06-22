# power-save-blocker

The `power-save-blocker` module is used to block the system from entering
low-power(sleep) mode, allowing app to keep system and screen active.

An example is:

```javascript
var powerSaveBlocker = require('power-save-blocker');

var id = powerSaveBlocker.start(powerSaveBlocker.PREVENT_DISPLAY_SLEEP);
console.log(powerSaveBlocker.isStarted(id));

powerSaveBlocker.stop(id);
```

## powerSaveBlocker.start(type)

* `type` - Power save blocker type
  * powerSaveBlocker.PREVENT_APP_SUSPENSION - Prevent the application from being
    suspended. Keeps system active, but allows screen to be turned off.
    Example use cases: downloading a file, playing audio.
  * powerSaveBlocker.PREVENT_DISPLAY_SLEEP - Prevent the display from going to sleep.
    Keeps system and screen active.
    Example use case: playing video.

Starts the power save blocker preventing the system entering lower-power mode.
Returns an integer identified the power save blocker.

**Note:**
`PREVENT_DISPLAY_SLEEP` has higher precedence level than `PREVENT_APP_SUSPENSION`.
Only the highest precedence type takes effect. In other words, `PREVENT_DISPLAY_SLEEP`
always take precedence over `PREVENT_APP_SUSPENSION`.

For example, an API calling A requests for `PREVENT_APP_SUSPENSION`, and
another calling B requests for `PREVENT_DISPLAY_SLEEP`. `PREVENT_DISPLAY_SLEEP`
will be used until B stops its request. After that, `PREVENT_APP_SUSPENSION` is used.

## powerSaveBlocker.stop(id)

* `id` Integer - The power save blocker id returned by `powerSaveBlocker.start`.

Stops the specified power save blocker.

## powerSaveBlocker.isStarted(id)

* `id` Integer - The power save blocker id returned by `powerSaveBlocker.start`.

Returns whether the corresponding `powerSaveBlocker` starts.

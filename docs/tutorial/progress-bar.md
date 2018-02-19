# Progress Bar in Taskbar (Windows, macOS, Unity)

On Windows a taskbar button can be used to display a progress bar. This enables
a window to provide progress information to the user without the user having to
switch to the window itself.

On macOS the progress bar will be displayed as a part of the dock icon.

The Unity DE also has a similar feature that allows you to specify the progress
bar in the launcher.

__Progress bar in taskbar button:__

![Taskbar Progress Bar][taskbar-progress-image]

All three cases are covered by the same API - the `setProgressBar()` method
available on instances of `BrowserWindows`. Call it with a number between `0`
and `1` to indicate your progress. If you have a long-running task that's
currently at 63% towards completion, you'd call it with `setProgressBar(0.63)`.

Generally speaking, setting the parameter to a value below zero (like `-1`)
will remove the progress bar while setting it to a value higher than one
(like `2`) will switch the progress bar to intermediate mode.

See the [API documentation for more options and modes][setprogressbar].

```javascript
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()

win.setProgressBar(0.5)
```

[taskbar-progress-image]: https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png
[setprogressbar]: ../api/browser-window.md#winsetprogressbarprogress

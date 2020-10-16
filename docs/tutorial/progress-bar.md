# Progress Bar in Taskbar (Windows, macOS, Unity)

## Overview

On Windows, a taskbar button can be used to display a progress bar. This enables
a window to provide progress information to the user without the need of switching to
the window itself.

![Taskbar Progress Bar][taskbar-progress-image]

On macOS, the progress bar will be displayed as a part of the dock icon.

The Unity DE also has a similar feature that allows you to specify the progress
bar in the launcher.

----

All three cases are covered by the same API - the
[`setProgressBar()`][setprogressbar] method available on an instance of
`BrowserWindow`. To indicate your progress, call this method with a number
between `0` and `1`. For example, if you have a long-running task that is
currently at 63% towards completion, you would call it as
`setProgressBar(0.63)`.

Setting the parameter to negative values (e.g. `-1`) will remove the progress
bar, whereas setting it to values greater than `1` (e.g. `2`) will switch the
progress bar to intermediate mode (100% complete).

See the [API documentation for more options and modes][setprogressbar].

## Example

Starting with a working application from the
[Quick Start Guide](quick-start.md), add the following lines to the
`main.js` file:

```javascript
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()

win.setProgressBar(0.5)
```

After launching the Electron application, you should see the bar in
the dock (macOS) or taskbar (Windows, Unity), indicating the progress
percentage you just defined.

![macOS dock progress bar](../images/dock-progress-bar.png)

For macOS, the progress bar will also be indicated for your application
when using [Mission control](https://support.apple.com/en-us/HT204100):

![Mission Control Progress Bar](../images/mission-control-progress-bar.png)

[taskbar-progress-image]: https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png
[setprogressbar]: ../api/browser-window.md#winsetprogressbarprogress

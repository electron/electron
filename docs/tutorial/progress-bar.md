# Progress Bar in Taskbar (Windows, macOS, Unity)

## Overview

A progress bar enables a window to provide progress information to the user
without the need of switching to the window itself.

On Windows, you can use a taskbar button to display a progress bar.

![Windows Progress Bar][windows-progress-bar]

On macOS, the progress bar will be displayed as a part of the dock icon.

![macOS Progress Bar][macos-progress-bar]

On Linux, the Unity graphical interface also has a similar feature that allows
you to specify the progress bar in the launcher.

![Linux Progress Bar][linux-progress-bar]

> NOTE: on Windows, each window can have its own progress bar, whereas on macOS
and Linux (Unity) there can be only one progress bar for the application.

----

All three cases are covered by the same API - the
[`setProgressBar()`][setprogressbar] method available on an instance of
`BrowserWindow`. To indicate your progress, call this method with a number
between `0` and `1`. For example, if you have a long-running task that is
currently at 63% towards completion, you would call it as
`setProgressBar(0.63)`.

Setting the parameter to negative values (e.g. `-1`) will remove the progress
bar. Setting it to a value greater than `1` will indicate an indeterminate progress bar
in Windows or clamp to 100% in other operating systems. An indeterminate progress bar
remains active but does not show an actual percentage, and is used for situations when
you do not know how long an operation will take to complete.

See the [API documentation for more options and modes][setprogressbar].

## Example

Add the following lines to the
`main.js` file:

```javascript fiddle='docs/fiddles/features/progress-bar'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()

const INCREMENT = 0.02
const INTERVAL_DELAY = 100 // ms
let c = 0

// create an interval timer that fires every INTERVAL_DELAY
// see docs at https://nodejs.org/api/timers.html#timers_setinterval_callback_delay_args
const interval = setInterval(() => {
    // update progress bar to next value
    win.setProgressBar(c)

    if (c > 1) {
        // progress bar has reached full so reset it and stop the timer
        win.setProgressBar(-1)
        clearTimeout(interval)
    }
    c += INCREMENT
}, INTERVAL_DELAY)

```

After launching the Electron application, the dock (macOS) or taskbar (Windows, Unity)
should show a progress bar that starts at zero and progresses through 100% to completion.

![macOS dock progress bar](../images/dock-progress-bar.png)

For macOS, the progress bar will also be indicated for your application
when using [Mission Control](https://support.apple.com/en-us/HT204100):

![Mission Control Progress Bar](../images/mission-control-progress-bar.png)

[windows-progress-bar]: https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png
[macos-progress-bar]: ../images/macos-progress-bar.png
[linux-progress-bar]: ../images/linux-progress-bar.png
[setprogressbar]: ../api/browser-window.md#winsetprogressbarprogress-options

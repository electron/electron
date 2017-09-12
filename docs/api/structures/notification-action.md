# NotificationAction Object

* `type` String - The type of action, can be `button`.
* `text` String - (optional) The label for the given action.

## Platform / Action Support

| Action Type | Platform Support | Usage of `text` | Default `text` | Limitations |
|-------------|------------------|-----------------|----------------|-------------|
| `button`    | macOS            | Used as the label for the button | "Show" | Maximum of one button, if multiple are provided only the last is used.  This action is also incomptible with `hasReply` and will be ignored if `hasReply` is `true`. |
| `button`    | Windows 10+      | Used as the label for the button | "Show" | N/A |

### Button support on macOS

In order for extra notification buttons to work on macOS your app must meet the
following criteria.

* App is signed
* App has it's `NSUserNotificationAlertStyle` set to `alert` in the `info.plist`.

If either of these requirements are not met the button simply won't appear.

### Button support on Windows

Buttons are only supported on Windows 10 and above, in order for buttons to appear
your app must meet the following criteria.

* App is running with `app.makeSingleInstance()`
* You have called `Notification.setWindowsProtocol(protocol)` with a protocol that Electron can register to support notification actions

**NOTE:** The provided protocol must **NOT** be in use by your app already,
this protocol must be dedicated to just this API or strange things may happen.
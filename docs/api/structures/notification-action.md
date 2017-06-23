# NotificationAction Object

* `type` String - The type of action, can be `button`.
* `label` String - The label for the given action.

## Platform / Action Support

| Action Type | Platform Support | Limitations |
|-------------|------------------|-------------|
| `button`    | macOS            | Maximum of one button, if multiple are provided only the last is used |

### Button support on macOS

In order for extra notification buttons to work on macOS your app must meet the
following criteria.

* App is signed
* App has it's `NSUserNotificationAlertStyle` set to `alert` in the `info.plist`.

If either of these requirements are not met the button simply won't appear.

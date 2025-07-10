# NotificationAction Object

* `type` string - The type of action, can be `button` or `selection`. `selection` is only supported on Windows.
* `text` string (optional) - The label for the given action.
* `items` string[] (optional) _Windows_ - The list of items for the `selection` action `type`.

## Platform / Action Support

| Action Type | Platform Support | Usage of `text` | Default `text` | Limitations |
|-------------|------------------|-----------------|----------------|-------------|
| `button`    | macOS, Windows   | Used as the label for the button | "Show" on macOS (localized) if first `button`, otherwise empty; Windows uses provided `text` | macOS: Only the first one is used as primary; others shown as additional actions (hover). Incompatible with `hasReply` (beyond first ignored). |
| `selection` | Windows          | Used as the label for the submit button for the selection menu | "Select" | Requires an `items` array property specifying option labels. Emits the `action` event with `(index, selectedIndex)` where `selectedIndex` is the chosen option (>= 0). Ignored on platforms that do not support selection actions. |

### Button support on macOS

In order for extra notification buttons to work on macOS your app must meet the
following criteria.

* App is signed
* App has its `NSUserNotificationAlertStyle` set to `alert` in the `Info.plist`.

If either of these requirements are not met the button won't appear.

### Selection support on Windows

To add a selection (combo box) style action, include an action with `type: 'selection'`, a `text` label for the submit button, and an `items` array of strings:

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const items = ['One', 'Two', 'Three']
  const n = new Notification({
    title: 'Choose an option',
    actions: [{
      type: 'selection',
      text: 'Apply',
      items
    }]
  })

  n.on('action', (e) => {
    console.log(`User triggered action at index: ${e.actionIndex}`)
    if (e.selectionIndex > 0) {
      console.log(`User chose selection item '${items[e.selectionIndex]}'`)
    }
  })

  n.show()
})
```

When the user activates the selection action, the notification's `action` event will be emitted with two parameters: `actionIndex` (the action's index in the `actions` array) and `selectedIndex` (the zero-based index of the chosen item, or `-1` if unavailable). On non-Windows platforms selection actions are ignored.

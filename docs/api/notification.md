# Notification

> Create OS desktop notifications

Process: [Main](../glossary.md#main-process)

> [!NOTE]
> If you want to show notifications from a renderer process you should use the
> [web Notifications API](../tutorial/notifications.md)

> [!NOTE]
> On MacOS, notifications use the UNNotification API as their underlying framework.
> This API requires an application to be code-signed in order for notifications
> to appear. Unsigned binaries will emit a `failed` event when notifications
> are called.

## Class: Notification

> Create OS desktop notifications

Process: [Main](../glossary.md#main-process)

`Notification` is an [EventEmitter][event-emitter].

It creates a new `Notification` with native properties as set by the `options`.

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### Static Methods

The `Notification` class has the following static methods:

#### `Notification.isSupported()`

Returns `boolean` - Whether or not desktop notifications are supported on the current system

#### `Notification.handleActivation(callback)` _Windows_

* `callback` Function
  * `details` [ActivationArguments](structures/activation-arguments.md) - Details about the notification activation.

Registers a callback to handle all notification activations. The callback is invoked whenever a
notification is clicked, replied to, or has an action button pressed - regardless of whether
the original `Notification` object is still in memory.

This method handles timing automatically:

* If an activation already occurred before calling this method, the callback is invoked immediately
  with those details.
* For all subsequent activations, the callback is invoked when they occur.

The callback remains registered until replaced by another call to `handleActivation`.

This provides a centralized way to handle notification interactions that works in all scenarios:

* Cold start (app launched from notification click)
* Notifications persisted in AC that have no in-memory representation after app re-start
* Notification object was garbage collected
* Notification object is still in memory (callback is invoked in addition to instance events)

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  // Register handler for all notification activations
  Notification.handleActivation((details) => {
    console.log('Notification activated:', details.type)
    if (details.type === 'reply') {
      console.log('User reply:', details.reply)
    } else if (details.type === 'action') {
      console.log('Action index:', details.actionIndex)
    }
  })
})
```

#### `Notification.getHistory()` _macOS_

Returns `Promise<NotificationHistoryInfo[]>` - Resolves with an array of [`NotificationHistoryInfo`](structures/notification-history-info.md) objects representing all delivered notifications still present in Notification Center.

> [!NOTE]
> Like all macOS notification APIs, this method requires the application to be
> code-signed. In unsigned development builds, notifications are not delivered
> to Notification Center and this method will resolve with an empty array.

```js
const { Notification, app } = require('electron')

app.whenReady().then(async () => {
  const history = await Notification.getHistory()
  for (const n of history) {
    console.log(`${n.id}: ${n.title}`)
  }
})
```

#### `Notification.remove(id)` _macOS_

* `id` (string | string[]) - The notification identifier(s) to remove. These correspond to the `id` values set in the [`Notification` constructor](#new-notificationoptions).

Removes one or more delivered notifications from Notification Center by their identifier(s).

```js
const { Notification } = require('electron')

// Remove a single notification
Notification.remove('my-notification-id')

// Remove multiple notifications
Notification.remove(['msg-1', 'msg-2', 'msg-3'])
```

#### `Notification.removeAll()` _macOS_

Removes all of the app's delivered notifications from Notification Center.

```js
const { Notification } = require('electron')

Notification.removeAll()
```

### `new Notification([options])`

* `options` Object (optional)
  * `id` string (optional) _macOS_ - A unique identifier for the notification, mapping to `UNNotificationRequest`'s [`identifier`](https://developer.apple.com/documentation/usernotifications/unnotificationrequest/identifier) property. Defaults to a random UUID if not provided or if an empty string is passed. Use this identifier with [`Notification.remove()`](#notificationremoveid-macos) to remove specific delivered notifications, or with [`Notification.getHistory()`](#notificationgethistory-macos) to identify them.
  * `groupId` string (optional) _macOS_ - A string identifier used to visually group notifications together in Notification Center. Maps to `UNNotificationContent`'s [`threadIdentifier`](https://developer.apple.com/documentation/usernotifications/unnotificationcontent/threadidentifier) property.
  * `title` string (optional) - A title for the notification, which will be displayed at the top of the notification window when it is shown.
  * `subtitle` string (optional) _macOS_ - A subtitle for the notification, which will be displayed below the title.
  * `body` string (optional) - The body text of the notification, which will be displayed below the title or subtitle.
  * `silent` boolean (optional) - Whether or not to suppress the OS notification noise when showing the notification.
  * `icon` (string | [NativeImage](native-image.md)) (optional) - An icon to use in the notification. If a string is passed, it must be a valid path to a local icon file.
  * `hasReply` boolean (optional) _macOS_ - Whether or not to add an inline reply option to the notification.
  * `timeoutType` string (optional) _Linux_ _Windows_ - The timeout duration of the notification. Can be 'default' or 'never'.
  * `replyPlaceholder` string (optional) _macOS_ - The placeholder to write in the inline reply input field.
  * `sound` string (optional) _macOS_ - The name of the sound file to play when the notification is shown.
  * `urgency` string (optional) _Linux_ - The urgency level of the notification. Can be 'normal', 'critical', or 'low'.
  * `actions` [NotificationAction[]](structures/notification-action.md) (optional) _macOS_ - Actions to add to the notification. Please read the available actions and limitations in the `NotificationAction` documentation.
  * `closeButtonText` string (optional) _macOS_ - A custom title for the close button of an alert. An empty string will cause the default localized text to be used.
  * `toastXml` string (optional) _Windows_ - A custom description of the Notification on Windows superseding all properties above. Provides full customization of design and behavior of the notification.

### Instance Events

Objects created with `new Notification` emit the following events:

:::info

Some events are only available on specific operating systems and are labeled as such.

:::

#### Event: 'show'

Returns:

* `event` Event

Emitted when the notification is shown to the user. Note that this event can be fired
multiple times as a notification can be shown multiple times through the
`show()` method.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Title!',
    subtitle: 'Subtitle!',
    body: 'Body!'
  })

  n.on('show', () => console.log('Notification shown!'))

  n.show()
})
```

#### Event: 'click'

Returns:

* `event` Event

Emitted when the notification is clicked by the user.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Title!',
    subtitle: 'Subtitle!',
    body: 'Body!'
  })

  n.on('click', () => console.log('Notification clicked!'))

  n.show()
})
```

#### Event: 'close'

Returns:

* `details` Event\<\>
  * `reason` _Windows_ string (optional) - The reason the notification was closed. This can be 'userCanceled', 'applicationHidden', or 'timedOut'.

Emitted when the notification is closed by manual intervention from the user.

This event is not guaranteed to be emitted in all cases where the notification
is closed.

On Windows, the `close` event can be emitted in one of three ways: programmatic dismissal with `notification.close()`, by the user closing the notification, or via system timeout. If a notification is in the Action Center after the initial `close` event is emitted, a call to `notification.close()` will remove the notification from the action center but the `close` event will not be emitted again.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Title!',
    subtitle: 'Subtitle!',
    body: 'Body!'
  })

  n.on('close', () => console.log('Notification closed!'))

  n.show()
})
```

#### Event: 'reply' _macOS_ _Windows_

Returns:

* `details` Event\<\>
  * `reply` string - The string the user entered into the inline reply field.
* `reply` string _Deprecated_

Emitted when the user clicks the "Reply" button on a notification with `hasReply: true`.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Send a Message',
    body: 'Body Text',
    hasReply: true,
    replyPlaceholder: 'Message text...'
  })

  n.on('reply', (e, reply) => console.log(`User replied: ${reply}`))
  n.on('click', () => console.log('Notification clicked'))

  n.show()
})
```

#### Event: 'action' _macOS_ _Windows_

Returns:

* `details` Event\<\>
  * `actionIndex` number - The index of the action that was activated.
  * `selectionIndex` number _Windows_ - The index of the selected item, if one was chosen. -1 if none was chosen.
* `actionIndex` number _Deprecated_
* `selectionIndex` number _Windows_ _Deprecated_

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const items = ['One', 'Two', 'Three']
  const n = new Notification({
    title: 'Choose an Action!',
    actions: [
      { type: 'button', text: 'Action 1' },
      { type: 'button', text: 'Action 2' },
      { type: 'selection', text: 'Apply', items }
    ]
  })

  n.on('click', () => console.log('Notification clicked'))
  n.on('action', (e) => {
    console.log(`User triggered action at index: ${e.actionIndex}`)
    if (e.selectionIndex > -1) {
      console.log(`User chose selection item '${items[e.selectionIndex]}'`)
    }
  })

  n.show()
})
```

#### Event: 'failed' _Windows_

Returns:

* `event` Event
* `error` string - The error encountered during execution of the `show()` method.

Emitted when an error is encountered while creating and showing the native notification.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Bad Action'
  })

  n.on('failed', (e, err) => {
    console.log('Notification failed: ', err)
  })

  n.show()
})
```

### Instance Methods

Objects created with the `new Notification()` constructor have the following instance methods:

#### `notification.show()`

Immediately shows the notification to the user. Unlike the web notification API,
instantiating a `new Notification()` does not immediately show it to the user. Instead, you need to
call this method before the OS will display it.

If the notification has been shown before, this method will dismiss the previously
shown notification and create a new one with identical properties.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Title!',
    subtitle: 'Subtitle!',
    body: 'Body!'
  })

  n.show()
})
```

#### `notification.close()`

Dismisses the notification.

On Windows, calling `notification.close()` while the notification is visible on screen will dismiss the notification and remove it from the Action Center. If `notification.close()` is called after the notification is no longer visible on screen, calling `notification.close()` will try remove it from the Action Center.

```js
const { Notification, app } = require('electron')

app.whenReady().then(() => {
  const n = new Notification({
    title: 'Title!',
    subtitle: 'Subtitle!',
    body: 'Body!'
  })

  n.show()

  setTimeout(() => n.close(), 5000)
})
```

### Instance Properties

#### `notification.id` _macOS_ _Readonly_

A `string` property representing the unique identifier of the notification. This is set at construction time — either from the `id` option or as a generated UUID if none was provided.

#### `notification.groupId` _macOS_ _Readonly_

A `string` property representing the group identifier of the notification. Notifications with the same `groupId` will be visually grouped together in Notification Center.

#### `notification.title`

A `string` property representing the title of the notification.

#### `notification.subtitle`

A `string` property representing the subtitle of the notification.

#### `notification.body`

A `string` property representing the body of the notification.

#### `notification.replyPlaceholder`

A `string` property representing the reply placeholder of the notification.

#### `notification.sound`

A `string` property representing the sound of the notification.

#### `notification.closeButtonText`

A `string` property representing the close button text of the notification.

#### `notification.silent`

A `boolean` property representing whether the notification is silent.

#### `notification.hasReply`

A `boolean` property representing whether the notification has a reply action.

#### `notification.urgency` _Linux_

A `string` property representing the urgency level of the notification. Can be 'normal', 'critical', or 'low'.

Default is 'low' - see [NotifyUrgency](https://developer-old.gnome.org/notification-spec/#urgency-levels) for more information.

#### `notification.timeoutType` _Linux_ _Windows_

A `string` property representing the type of timeout duration for the notification. Can be 'default' or 'never'.

If `timeoutType` is set to 'never', the notification never expires. It stays open until closed by the calling API or the user.

#### `notification.actions`

A [`NotificationAction[]`](structures/notification-action.md) property representing the actions of the notification.

#### `notification.toastXml` _Windows_

A `string` property representing the custom Toast XML of the notification.

### Playing Sounds

On macOS, you can specify the name of the sound you'd like to play when the
notification is shown. Any of the default sounds (under System Preferences >
Sound) can be used, in addition to custom sound files. Be sure that the sound
file is copied under the app bundle (e.g., `YourApp.app/Contents/Resources`),
or one of the following locations:

* `~/Library/Sounds`
* `/Library/Sounds`
* `/Network/Library/Sounds`
* `/System/Library/Sounds`

See the [`NSSound`](https://developer.apple.com/documentation/appkit/nssound) docs for more information.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter

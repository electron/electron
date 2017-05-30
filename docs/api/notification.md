# Notification

> Create OS desktop notifications

Process: [Main](../glossary.md#main-process)

## Using in the renderer process

If you want to show Notifications from a renderer process you should use the [HTML5 Notification API](../tutorial/notifications.md)

## Class: Notification

> Create OS desktop notifications

Process: [Main](../glossary.md#main-process)

`Notification` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

It creates a new `Notification` with native properties as set by the `options`.

### Static Methods

The `Notification` class has the following static methods:

#### `BrowserWindow.isSupported()`

Returns `Boolean` - Whether or not desktop notifications are supported on the current system

### `new Notification([options])` _Experimental_

* `options` Object
  * `title` String - A title for the notification, which will be shown at the top of the notification window when it is shown
  * `body` String - The body text of the notification, which will be displayed below the title
  * `silent` Boolean - (Optional) Whether or not to emit an OS notification noise when showing the notification
  * `icon` [NativeImage](native-image.md) - (Optional) An icon to use in the notification
  * `hasReply` Boolean - (Optional) Whether or not to add an inline reply option to the notification.  _macOS_
  * `replyPlaceholder` String - (Optional) The placeholder to write in the inline reply input field. _macOS_


### Instance Events

Objects created with `new Notification` emit the following events:

**Note:** Some events are only available on specific operating systems and are
labeled as such.

#### Event: 'show'

Returns:

* `event` Event

Emitted when the notification is shown to the user, note this could be fired
multiple times as a notification can be shown multiple times through the
`show()` method.

#### Event: 'click'

Returns:

* `event` Event

Emitted when the notification is clicked by the user.

#### Event: 'close'

Returns:

* `event` Event

Emitted when the notification is closed by manual intervention from the user.

This event is not guarunteed to be emitted in all cases where the notification
is closed.

#### Event: 'reply' _macOS_

Returns:

* `event` Event
* `reply` String - The string the user entered into the inline reply field

Emitted when the user clicks the "Reply" button on a notification with `hasReply: true`.

### Instance Methods

Objects created with `new Notification` have the following instance methods:

#### `notification.show()`

Immediately shows the notification to the user, please note this means unlike the
HTML5 Notification implementation, simply instantiating a `new Notification` does
not immediately show it to the user, you need to call this method before the OS
will display it.

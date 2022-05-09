# pushNotifications

> Register for remote push notification services

Process: [Main](../glossary.md#main-process)

## Events

The `pushNotification` module emits the following events:

#### Event: 'registered-for-apns-notifications' _macOS_

Returns:

* `token` String - A token that identifies the device to Apple Push Notification Service (APNS)

Emitted when the app successfully registers to receive remote notifications from APNS.
See: https://developer.apple.com/documentation/appkit/nsapplicationdelegate/1428766-application?language=objc

#### Event: 'failed-to-register-for-apns-notifications' _macOS_

Returns:

* `error` String

Emitted when the app fails to register to receive remote notifications from APNS.
See: https://developer.apple.com/documentation/appkit/nsapplicationdelegate/1428554-application?language=objc

#### Event: 'received-apns-notification' _macOS_

Returns:

* `userInfo` Record<String, any>

Emitted when the app receives a remote notification while running.
See: https://developer.apple.com/documentation/appkit/nsapplicationdelegate/1428430-application?language=objc

## Methods

The `pushNotification` module has the following methods:

### `pushNotifications.registerForAPNSNotifications()` _macOS_

Registers the app with Apple Push Notification service (APNS) to receive [Badge, Sound, and Alert](https://developer.apple.com/documentation/appkit/sremotenotificationtype?language=objc) notifications. If registration is successful, the BrowserWindow event `registered-for-apns-notifications` will be emitted with the APNS device token. Otherwise, the event `failed-to-register-for-apns-notifications` will be emitted.
See: https://developer.apple.com/documentation/appkit/nsapplication/1428476-registerforremotenotificationtyp?language=objc

### `pushNotifications.unregisterForAPNSNotifications()` _macOS_

Unregisters the app from notifications received from APNS.
See: https://developer.apple.com/documentation/appkit/nsapplication/1428747-unregisterforremotenotifications?language=objc

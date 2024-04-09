# Device Access

Like Chromium based browsers, Electron provides access to device hardware
through web APIs.  For the most part these APIs work like they do in a browser,
but there are some differences that need to be taken into account.  The primary
difference between Electron and browsers is what happens when device access is
requested.  In a browser, users are presented with a popup where they can grant
access to an individual device.  In Electron APIs are provided which can be
used by a developer to either automatically pick a device or prompt users to
pick a device via a developer created interface.

## Web Bluetooth API

The [Web Bluetooth API](https://web.dev/bluetooth/) can be used to communicate
with bluetooth devices. In order to use this API in Electron, developers will
need to handle the [`select-bluetooth-device` event on the webContents](../api/web-contents.md#event-select-bluetooth-device)
associated with the device request.

Additionally, [`ses.setBluetoothPairingHandler(handler)`](../api/session.md#sessetbluetoothpairinghandlerhandler-windows-linux)
can be used to handle pairing to bluetooth devices on Windows or Linux when
additional validation such as a pin is needed.

### Example

This example demonstrates an Electron application that automatically selects
the first available bluetooth device when the `Test Bluetooth` button is
clicked.

```fiddle docs/fiddles/features/web-bluetooth

```

## WebHID API

The [WebHID API](https://web.dev/hid/) can be used to access HID devices such
as keyboards and gamepads. Electron provides several APIs for working with
the WebHID API:

* The [`select-hid-device` event on the Session](../api/session.md#event-select-hid-device)
  can be used to select a HID device when a call to
  `navigator.hid.requestDevice` is made.  Additionally the [`hid-device-added`](../api/session.md#event-hid-device-added)
  and [`hid-device-removed`](../api/session.md#event-hid-device-removed) events
  on the Session can be used to handle devices being plugged in or unplugged
  when handling the `select-hid-device` event.
  **Note:** These events only fire until the callback from `select-hid-device`
  is called.  They are not intended to be used as a generic hid device listener.
* [`ses.setDevicePermissionHandler(handler)`](../api/session.md#sessetdevicepermissionhandlerhandler)
  can be used to provide default permissioning to devices without first calling
  for permission to devices via `navigator.hid.requestDevice`.  Additionally,
  the default behavior of Electron is to store granted device permission through
  the lifetime of the corresponding WebContents.  If longer term storage is
  needed, a developer can store granted device permissions (eg when handling
  the `select-hid-device` event) and then read from that storage with
  `setDevicePermissionHandler`.
* [`ses.setPermissionCheckHandler(handler)`](../api/session.md#sessetpermissioncheckhandlerhandler)
  can be used to disable HID access for specific origins.

### Blocklist

By default Electron employs the same [blocklist](https://github.com/WICG/webhid/blob/main/blocklist.txt)
used by Chromium.  If you wish to override this behavior, you can do so by
setting the `disable-hid-blocklist` flag:

```js
app.commandLine.appendSwitch('disable-hid-blocklist')
```

### Example

This example demonstrates an Electron application that automatically selects
HID devices through [`ses.setDevicePermissionHandler(handler)`](../api/session.md#sessetdevicepermissionhandlerhandler)
and through [`select-hid-device` event on the Session](../api/session.md#event-select-hid-device)
when the `Test WebHID` button is clicked.

```fiddle docs/fiddles/features/web-hid

```

## Web Serial API

The [Web Serial API](https://web.dev/serial/) can be used to access serial
devices that are connected via serial port, USB, or Bluetooth.  In order to use
this API in Electron, developers will need to handle the
[`select-serial-port` event on the Session](../api/session.md#event-select-serial-port)
associated with the serial port request.

There are several additional APIs for working with the Web Serial API:

* The [`serial-port-added`](../api/session.md#event-serial-port-added)
  and [`serial-port-removed`](../api/session.md#event-serial-port-removed) events
  on the Session can be used to handle devices being plugged in or unplugged
  when handling the `select-serial-port` event.
  **Note:** These events only fire until the callback from `select-serial-port`
  is called.  They are not intended to be used as a generic serial port
  listener.
* [`ses.setDevicePermissionHandler(handler)`](../api/session.md#sessetdevicepermissionhandlerhandler)
  can be used to provide default permissioning to devices without first calling
  for permission to devices via `navigator.serial.requestPort`.  Additionally,
  the default behavior of Electron is to store granted device permission through
  the lifetime of the corresponding WebContents.  If longer term storage is
  needed, a developer can store granted device permissions (eg when handling
  the `select-serial-port` event) and then read from that storage with
  `setDevicePermissionHandler`.
* [`ses.setPermissionCheckHandler(handler)`](../api/session.md#sessetpermissioncheckhandlerhandler)
  can be used to disable serial access for specific origins.

### Example

This example demonstrates an Electron application that automatically selects
serial devices through [`ses.setDevicePermissionHandler(handler)`](../api/session.md#sessetdevicepermissionhandlerhandler)
as well as demonstrating selecting the first available Arduino Uno serial device (if connected) through
[`select-serial-port` event on the Session](../api/session.md#event-select-serial-port)
when the `Test Web Serial` button is clicked.

```fiddle docs/fiddles/features/web-serial

```

## WebUSB API

The [WebUSB API](https://web.dev/usb/) can be used to access USB devices.
Electron provides several APIs for working with the WebUSB API:

* The [`select-usb-device` event on the Session](../api/session.md#event-select-usb-device)
  can be used to select a USB device when a call to
  `navigator.usb.requestDevice` is made.  Additionally the [`usb-device-added`](../api/session.md#event-usb-device-added)
  and [`usb-device-removed`](../api/session.md#event-usb-device-removed) events
  on the Session can be used to handle devices being plugged in or unplugged
  when handling the `select-usb-device` event.
  **Note:** These two events only fire until the callback from `select-usb-device`
  is called.  They are not intended to be used as a generic usb device listener.
* The [`usb-device-revoked` event on the Session](../api/session.md#event-usb-device-revoked) can
  be used to respond when [device.forget()](https://developer.chrome.com/articles/usb/#revoke-access)
  is called on a USB device.
* [`ses.setDevicePermissionHandler(handler)`](../api/session.md#sessetdevicepermissionhandlerhandler)
  can be used to provide default permissioning to devices without first calling
  for permission to devices via `navigator.usb.requestDevice`.  Additionally,
  the default behavior of Electron is to store granted device permission through
  the lifetime of the corresponding WebContents.  If longer term storage is
  needed, a developer can store granted device permissions (eg when handling
  the `select-usb-device` event) and then read from that storage with
  `setDevicePermissionHandler`.
* [`ses.setPermissionCheckHandler(handler)`](../api/session.md#sessetpermissioncheckhandlerhandler)
  can be used to disable USB access for specific origins.
* [`ses.setUSBProtectedClassesHandler](../api/session.md#sessetusbprotectedclasseshandlerhandler)
  can be used to allow usage of [protected USB classes](https://wicg.github.io/webusb/#usbinterface-interface) that are not available by default.

### Example

This example demonstrates an Electron application that automatically selects
USB devices (if they are attached) through [`ses.setDevicePermissionHandler(handler)`](../api/session.md#sessetdevicepermissionhandlerhandler)
and through [`select-usb-device` event on the Session](../api/session.md#event-select-usb-device)
when the `Test WebUSB` button is clicked.

```fiddle docs/fiddles/features/web-usb

```

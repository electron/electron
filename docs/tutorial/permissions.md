# Web Permissions in Electron

Web content requests capabilities such as camera, geolocation, notifications,
HID/USB/serial devices or opening a URL in another application through the
same web APIs it uses in a browser. In a browser the user answers those
requests through prompts and choosers. In Electron the app answers them in the
main process through `session` handlers and events.

This guide describes what those handlers are told about the requester, where
that differs from Chrome, and how to get Chrome's behaviour if you want it.
The APIs themselves are documented in [`session`](../api/session.md) and
[Device Access](./devices.md).

## Defaults

* With no handlers installed, permission checks and permission requests are
  allowed.
* With no `select-hid-device` / `select-usb-device` / `select-serial-port` /
  `select-bluetooth-device` listener, a device request returns no device (an
  empty list for HID, a `NotFoundError` for USB, serial and Bluetooth).
* Once you install `setPermissionCheckHandler`, `setPermissionRequestHandler`
  or `setDevicePermissionHandler`, its return value is the decision for every
  call routed to it.

## What each handler receives

Each handler or event identifies the document that made the request. For a
request from an `<iframe>`, a popup or a `<webview>` guest, the fields below
describe that frame, not the top-level page that contains it.

| API | Requester fields |
| --- | --- |
| [`ses.setPermissionCheckHandler`](../api/session.md#sessetpermissioncheckhandlerhandler) | `webContents`, `requestingOrigin`, `details.requestingUrl`, `details.isMainFrame`, `details.embeddingOrigin`, `details.frame` |
| [`ses.setPermissionRequestHandler`](../api/session.md#sessetpermissionrequesthandlerhandler) | `webContents`, `details.requestingUrl`, `details.isMainFrame` |
| `select-hid-device`, `select-usb-device`, `select-bluetooth-device` (session), `bluetooth-device-added` | `details.frame` |
| `select-serial-port`, `serial-port-added`, `serial-port-removed`, `usb-device-added`, `usb-device-removed` | `webContents` and a trailing `frame` argument |
| `select-bluetooth-device` (webContents, deprecated) | the `webContents` the event is emitted on and a trailing `frame` argument |
| [`ses.setDevicePermissionHandler`](../api/session.md#sessetdevicepermissionhandlerhandler) | `details.origin`, `details.frame`, `details.selected` |
| [`ses.setUSBProtectedClassesHandler`](../api/session.md#sessetusbprotectedclasseshandlerhandler) | `details.origin`, `details.frame` |
| `hid-device-revoked`, `usb-device-revoked`, `serial-port-revoked` | `details.origin`, `details.frame` |

Notes on the fields:

* `frame` is a [`WebFrameMain`](../api/web-frame-main.md), or `null` when the
  request was not made by a document (for example a service worker) or the
  frame has since been destroyed.
* `webContents` is the WebContents that contains the requesting frame. For an
  iframe that is the top-level page's WebContents; use `frame`,
  `requestingOrigin` and `isMainFrame` to tell frames apart.
* `requestingOrigin` and `details.origin` are the requesting document's origin.
  `embeddingOrigin` is the origin of the top-level document it is embedded in
  (equal to `requestingOrigin` for a main frame).
* `requestingUrl` / `frame.url` is the document's URL. Base decisions on the
  origin, not the URL: for `about:blank`, `blob:` and sandboxed documents the
  URL does not identify who controls the document.

### Opaque origins

A document in `<iframe sandbox>` without `allow-same-origin`, or loaded from a
`data:` URL, has an opaque origin. For such a requester:

* `requestingOrigin` is `''` and `details.origin` is `'null'`;
* `frame.origin` is `'null'`;
* `details.requestingUrl` / `frame.url` is the URL the document was loaded
  from;
* `frame.parent` / `frame.top` identify the embedding documents.

An opaque origin is not a stable key. Decide from `frame.parent.origin` /
`frame.top.origin`, or deny, and do not store grants under `'null'`.

## Differences from Chrome

Chrome follows the [Permissions specification](https://www.w3.org/TR/permissions/)
default: a permission is granted to the **top-level site**. A cross-origin
iframe can use it only if the top-level document delegates it with the
[`allow` attribute](https://developer.mozilla.org/en-US/docs/Web/HTTP/Permissions_Policy),
for example `<iframe allow="camera; hid">`. Chrome's prompts and device choosers
name the top-level site, and the grant is stored under the top-level origin.

Electron keeps the same precondition — a cross-origin iframe cannot call
`navigator.hid.requestDevice()` or `getUserMedia()` unless its embedder set
`allow=` for that feature — and differs in what it reports and stores:

* Handlers receive the requesting frame and its origin, not the top-level
  site's.
* A HID/USB/serial device picked in a chooser is granted to the requesting
  frame's origin. Other origins in the same page do not share that grant.
* `device.forget()` revokes the grant for the calling frame's origin.

### Using Chrome's model

To decide and store per top-level site instead, derive the key from the frame:

```js @ts-type={isDeviceAllowedForSite:(site:string|null,device:any)=>boolean}
const { session } = require('electron')

function topLevelOrigin (details, webContents) {
  if (details.frame) return details.frame.top.origin
  return webContents ? webContents.mainFrame.origin : null
}

session.defaultSession.setPermissionCheckHandler((webContents, permission, requestingOrigin, details) => {
  return topLevelOrigin(details, webContents) === 'https://app.example.com'
})

session.defaultSession.setDevicePermissionHandler((details) => {
  const site = details.frame ? details.frame.top.origin : details.origin
  return details.selected || isDeviceAllowedForSite(site, details.device)
})
```

Because the `allow=` precondition still applies, a decision keyed on
`frame.top.origin` matches Chrome's: the iframe only reaches the handler if the
top-level document delegated the feature to it.

## HID, USB, serial and Bluetooth in detail

1. **Check.** `setPermissionCheckHandler` is called with `hid`, `usb`,
   `serial` or `bluetooth` for the requesting frame before a chooser opens and before
   previously granted devices are returned by `getDevices()` or (HID, serial)
   opened. Returning `false` blocks all of these.
2. **Choose.** `select-hid-device` / `select-usb-device` / `select-serial-port`
   is emitted with the device list and the requesting frame. Calling back with
   a device id grants that device to the requesting frame's origin. Electron
   records the selection in memory; it is not written to disk.
3. **Use.** When the page calls `getDevices()` or opens a device,
   `setDevicePermissionHandler` (if installed) is called with the frame, its
   origin, the device, and `selected: true` if step 2 picked this device for
   this origin in the current session. Without a handler, Electron's record
   from step 2 decides.
4. **Revoke.** `device.forget()` removes the grant for the calling frame's
   origin, emits `hid-device-revoked` / `usb-device-revoked` /
   `serial-port-revoked` with that origin and frame, and closes the device in
   other documents of that origin.

Web Bluetooth has steps 1 and 2 only: the `bluetooth` check runs for the
requesting frame on every Web Bluetooth call, and `select-bluetooth-device` /
`bluetooth-device-added` carry the requesting frame. A device picked there is
usable by the requesting document; Electron keeps no grant record for it, so
`setDevicePermissionHandler` and the revoke step do not apply to Bluetooth.

In Chrome, steps 2–4 are keyed on the top-level site and there is no
equivalent of `setDevicePermissionHandler`.

## Checklist

* Install `setPermissionRequestHandler` and `setPermissionCheckHandler` in
  every session that loads remote content; deny by default and allow specific
  origins.
* Decide on `requestingOrigin` / `frame.origin` (or `frame.top.origin` for
  Chrome's model), not on `webContents.getURL()` or `requestingUrl`.
* Add `allow="…"` only to iframes you intend to delegate a capability to.
* If you install `setDevicePermissionHandler`, return `true` when
  `details.selected` is `true` unless you intend to veto a device the user just
  picked.
* Treat `frame.origin === 'null'` as untrusted unless you decide otherwise from
  `frame.parent`.

# shortcut

A `Shortcut` presents a global keyboard shortcut in operating system. If a 
`Shortcut` is registered in app, the app will receive an `active` event when
user presses the shortcut. Note that it is global, even your app does not get 
focused, it still works.


```javascript
var Shortcut = require('shortcut');

shortcut = new Shortcut('ctrl+a');
shortcut.setKey('ctrl+s');
shortcut.on('active', function() { console.log('ctrl+s pressed'); });
shortcut.on('failed', function() { console.log("failed"); });
shortcut.register();
```

## Class: Shortcut 

`Shortcut` is an [EventEmitter](event-emitter).

### new Shortcut(keycode)

* `keycode` String

Creates a new `Shortcut` associated with the `keycode`.

`keycode` is a string to specify shortcut key, such as "ctrl+shift+a".

A `keycode` consists of modifier and key two parts:

__Modifiers__: control(ctrl), command(cmd), alt, shift, commandorcontrol(cmdorctrl).

__Supported keys__: 0-9, a-z, up, down, left, right, home, end, pagedown, pageup,
insert, delete, esc, space, backspace, tab, f1-f12, volumeup, volumedown, media 
keys(medianextrack, mediaprevioustrack, mediastop, mediaplaypause).

### Event: active

Emitted when a registered `shortcut` is pressed by user.

### Event: failed

Emitted when the keycode of `shortcut` is invalid.

### Shortcut.setKey(keycode)

* `keycode` String 

Set new `keycode` to a `Shortcut`. Note that this operation will override previous
keycode and will unregister the `Shortcut`, developer should register the
`Shortcut` again after `setKey`.

### Shortcut.register

Register a `Shortcut` to operating system.

### Shortcut.unregister

Unregister a `Shortcut` to operating system.

### Shortcut.isRegistered

Return whether the shortcut is registered.

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter

# global-shortcut

The `global-shortcut` module can register/unregister a global keyboard shortcut
in operating system, so that you can custom the operations for various shortcuts.
Note that it is global, even the app does not get focused, it still works.

```javascript
var globalShortcut = require('global-shortcut');

// Register a 'ctrl+x' shortcut listener.
var ret = globalShortcut.register('ctrl+x', function() { console.log('ctrl+x is pressed'); })
if (!ret)
  console.log('registerion fails');

// Check whether a shortcut is registered.
console.log(globalShortcut.isRegistered('ctrl+x'));

// Unregister a shortcut.
globalShortcut.unregister('ctrl+x');

// Unregister all shortcuts.
globalShortcut.unregisterAll();
```

## globalShortcut.register(keycode, callback)

* `keycode` String
* `callback` Function

Registers a global shortcut of `keycode`, the `callback` would be called when
the registered shortcut is pressed by user.

`keycode` is a string to specify shortcut key, such as "ctrl+shift+a".

A `keycode` consists of modifier and key two parts:

__Modifiers__: control(ctrl), command(cmd), alt, shift, commandorcontrol(cmdorctrl).

__Supported keys__: 0-9, a-z, up, down, left, right, home, end, pagedown, pageup,
insert, delete, esc, space, backspace, tab, f1-f12, volumeup, volumedown, media 
keys(medianextrack, mediaprevioustrack, mediastop, mediaplaypause).

## globalShortcut.isRegistered(keycode)

* `keycode` String

Return whether the shortcut is registered.

## globalShortcut.unregister(keycode)

* `keycode` String

Unregisters the global shortcut of `keycode`.

## globalShortcut.unregisterAll()

Unregisters all the global shortcuts.

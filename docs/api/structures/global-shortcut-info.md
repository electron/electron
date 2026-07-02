# GlobalShortcutInfo Object

* `id` string - The identifier of the shortcut within the desktop portal
  session.
* `accelerator` string (optional) - The normalized
  [accelerator](../../tutorial/keyboard-shortcuts.md#accelerators) string for
  shortcuts registered by the `globalShortcut` module, e.g. `Alt+Shift+K`.
  Not present when the shortcut id does not follow the module's naming
  scheme, e.g. when the accelerator contains function keys.
* `description` string - Human-readable description provided when the
  shortcut was bound.
* `triggerDescription` string - Human-readable description of the key
  combination as reported by the desktop portal, e.g. `CTRL+SHIFT+K`. May be
  an empty string if the compositor does not assign a trigger, e.g. for a key
  combination it reserves.

# Accelerator

Define keyboard shortcuts.

Accelerators can contain multiple modifiers and key codes, combined by
the `+` character.

Examples:

* `CommandOrControl+A`
* `CommandOrControl+Shift+Z`

## Platform notice

On Linux and Windows, the `Command` key does not have any effect so
use `CommandOrControl` which represents `Command` on OS X and `Control` on
Linux and Windows to define some accelerators.

Use `Alt` instead of `Option`. The `Option` key only exists on OS X, whereas
the `Alt` key is available on all platforms.

The `Super` key is mapped to the `Windows` key on Windows and Linux and
`Cmd` on OS X.

## Available modifiers

* `Command` (or `Cmd` for short)
* `Control` (or `Ctrl` for short)
* `CommandOrControl` (or `CmdOrCtrl` for short)
* `Alt`
* `Option`
* `AltGr`
* `Shift`
* `Super`

## Available key codes

* `0` to `9`
* `A` to `Z`
* `F1` to `F24`
* Punctuations like `~`, `!`, `@`, `#`, `$`, etc.
* `Plus`
* `Space`
* `Backspace`
* `Delete`
* `Insert`
* `Return` (or `Enter` as alias)
* `Up`, `Down`, `Left` and `Right`
* `Home` and `End`
* `PageUp` and `PageDown`
* `Escape` (or `Esc` for short)
* `VolumeUp`, `VolumeDown` and `VolumeMute`
* `MediaNextTrack`, `MediaPreviousTrack`, `MediaStop` and `MediaPlayPause`
* `PrintScreen`

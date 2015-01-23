# Accelerator

An accelerator is string that represents a keyboard shortcut, it can contain
multiple modifiers and key codes, combined by the `+` character.

Examples:

* `Command+A`
* `Ctrl+Shift+Z`

## Platform notice

On Linux and Windows, the `Command` key would not have any effect, you can
use `CommandOrControl` which represents `Command` on OS X and `Control` on
Linux and Windows to define some accelerators.

## Available modifiers

* `Command` (or `Cmd` for short)
* `Control` (or `Ctrl` for short)
* `CommandOrControl` (or `CmdOrCtrl` for short)
* `Alt`
* `Shift`

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

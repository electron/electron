# Accelerator

Accelerator는 키보드 단축키를 표현하는 문자열입니다, 여러 혼합키와 키코드를 `+` 문자를
이용하여 결합할 수 있습니다.

예제:

* `Command+A`
* `Ctrl+Shift+Z`

## 플랫폼에 관련하여 주의할 점

Linux와 Windows에서는 `Command`키가 없으므로 작동하지 않습니다. 대신에 `CommandOrControl`을
사용하면 OS X의 `Command`와 Linux, Windows의 `Control` 모두 지원할 수 있습니다.

`Super`키는 Windows와 Linux에서는 `윈도우`키를, OS X에서는 `Cmd`키로 맵핑됩니다.

## 사용 가능한 혼합키

* `Command` (or `Cmd` for short)
* `Control` (or `Ctrl` for short)
* `CommandOrControl` (or `CmdOrCtrl` for short)
* `Alt`
* `Shift`
* `Super`

## 사용 가능한 전체 키코드

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

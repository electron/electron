# Accelerator

> 키보드 단축키를 정의합니다.

Accelerator 는 `+` 문자로 결합된 여러 수식어와 키 코드를 포함할 수 있는
문자열입니다. 그리고 애플리케이션의 키보드 단축키를 정의하는데 사용됩니다.

예시:

* `CommandOrControl+A`
* `CommandOrControl+Shift+Z`

단축키는 [`globalShortcut`](global-shortcut.md) 모듈의
[`register`](global-shortcut.md#globalshortcutregisteraccelerator-callback)
메소드로 등록됩니다. 예시:

```javascript
const {app, globalShortcut} = require('electron')

app.on('ready', () => {
  // '커맨드 또는 컨트롤+Y' 단축키 리스너 등록.
  globalShortcut.register('CommandOrControl+Y', () => {
    // 커맨드/컨트롤과 Y 가 눌렸을 때 할 동작.
  })
})
```

## 플랫폼에 관련하여 주의할 점

Linux와 Windows에서는 `Command`키가 없으므로 작동하지 않습니다. 대신에
`CommandOrControl`을 사용하면 macOS의 `Command`와 Linux, Windows의 `Control` 모두
지원할 수 있습니다.

`Option` 대신 `Alt`을 사용하는게 좋습니다. `Option` 키는 macOS에만 있으므로
모든 플랫폼에서 사용할 수 있는 `Alt` 키를 권장합니다.

`Super`키는 Windows와 Linux 에서는 `윈도우`키를, macOS에서는 `Cmd`키로 맵핑됩니다.

## 사용 가능한 혼합키

* `Command` (단축어 `Cmd`)
* `Control` (단축어 `Ctrl`)
* `CommandOrControl` (단축어 `CmdOrCtrl`)
* `Alt`
* `Option`
* `AltGr`
* `Shift`
* `Super`

## 사용 가능한 전체 키코드

* `0` 부터 `9` 까지
* `A` 부터 `Z` 까지
* `F1` 부터 `F24` 까지
* `~`, `!`, `@`, `#`, `$`, etc 와 같은 구두점 기호들
* `Plus`
* `Space`
* `Tab`
* `Backspace`
* `Delete`
* `Insert`
* `Return` (또는 `Enter`)
* `Up`, `Down`, `Left` 와 `Right`
* `Home` 그리고 `End`
* `PageUp` 그리고 `PageDown`
* `Escape` (단축어 `Esc`)
* `VolumeUp`, `VolumeDown` 그리고 `VolumeMute`
* `MediaNextTrack`, `MediaPreviousTrack`, `MediaStop` 그리고 `MediaPlayPause`
* `PrintScreen`

__키코드는 `단축어`로도 사용할 수 있습니다__

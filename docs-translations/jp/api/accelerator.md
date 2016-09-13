# Accelerator

acceleratorは、キーボードショートカットを示す文字列です。複数の修飾語句とキーコードを `+` 文字で結合します。

例:

* `Command+A`
* `Ctrl+Shift+Z`

## プラットフォームの留意点


macOSでは`Command` キー、LinuxとWindowsでは`Control` キーを意味する`CommandOrControl`はいくつかのacceleratorを定義しますが、LinuxとWindowsでは、`Command` キーは何の効果もありません。

 `Super` キーは、WindowsとLinuxでは `Windows` キーに、macOSでは、`Cmd` キーに関連付けられます。

## 提供されている修飾語句

* `Command` (または、短く `Cmd`)
* `Control` (または、短く `Ctrl`)
* `CommandOrControl` (または、短く `CmdOrCtrl`)
* `Alt`
* `Shift`
* `Super`

## 提供されているキーコード

* `0` to `9`
* `A` to `Z`
* `F1` to `F24`
* `~`, `!`, `@`, `#`, `$`などの記号
* `Plus`
* `Space`
* `Backspace`
* `Delete`
* `Insert`
* `Return` (またはエイリアスで `Enter`)
* `Up`と `Down`,`Left`、 `Right`
* `Home` と `End`
* `PageUp` と `PageDown`
* `Escape` (または、短く `Esc`)
* `VolumeUp`と `VolumeDown` 、 `VolumeMute`
* `MediaNextTrack`と `MediaPreviousTrack`、 `MediaStop` 、 `MediaPlayPause`

# Acelerador (teclas de atalhos)

Um acelerador é uma string que representa um atalho de tecla. Isso pode conter
multiplos modificadores e códigos chaves, combinado pelo caracter `+`.

Exemplos:

* `Command+A`
* `Ctrl+Shift+Z`

## Aviso sobre plataformas

No Linux e no Windows a tecla `Command` não tem nenhum efeito,
então use `CommandOrControl` que representa a tecla `Command` existente no OSX e
`Control` no Linux e no Windows para definir aceleradores (atalhos).

A chave `Super` está mapeada para a tecla `Windows` para Windows e Linux, 
e para a tecla `Cmd` para OSX.

## Modificadores disponiveis

* `Command` (ou `Cmd` abreviado)
* `Control` (ou `Ctrl` abreviado)
* `CommandOrControl` (ou `CmdOrCtrl` abreviado)
* `Alt`
* `Shift`
* `Super`

## Códigos chaves disponiveis

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
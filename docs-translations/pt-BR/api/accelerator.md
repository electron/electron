# Acelerador (teclas de atalhos)

Um acelerador é uma string que representa um atalho de tecla. Ele pode conter
múltiplos modificadores e códigos chaves, combinados pelo caractere `+`.

Exemplos:

* `Command+A`
* `Ctrl+Shift+Z`

## Aviso sobre plataformas

No Linux e no Windows a tecla `Command` não tem nenhum efeito,
então use `CommandOrControl` que representa a tecla `Command` existente no macOS e
`Control` no Linux e no Windows para definir aceleradores (atalhos).

A chave `Super` está mapeada para a tecla `Windows` para Windows e Linux, 
e para a tecla `Cmd` para macOS.

## Modificadores disponíveis

* `Command` (ou `Cmd` abreviado)
* `Control` (ou `Ctrl` abreviado)
* `CommandOrControl` (ou `CmdOrCtrl` abreviado)
* `Alt`
* `Shift`
* `Super`

## Códigos chaves disponíveis

* `0` até `9`
* `A` até `Z`
* `F1` até `F24`
* Pontuações como `~`, `!`, `@`, `#`, `$`, etc.
* `Plus`
* `Space`
* `Backspace`
* `Delete`
* `Insert`
* `Return` (ou `Enter` como pseudônimo)
* `Up`, `Down`, `Left` e `Right`
* `Home` e `End`
* `PageUp` e `PageDown`
* `Escape` (ou `Esc` abreviado)
* `VolumeUp`, `VolumeDown` e `VolumeMute`
* `MediaNextTrack`, `MediaPreviousTrack`, `MediaStop` e `MediaPlayPause`

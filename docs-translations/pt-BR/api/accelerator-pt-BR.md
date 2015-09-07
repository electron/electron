# Acelerador

Um acelerador é uma string que representa um atalho de tecla. Isso pode conter
multiplos modificadores e códigos chaves, combinado pelo caracter  `+`.

Exemplos:

* `Command+A`
* `Ctrl+Shift+Z`

## Aviso sobre as plataformas (Linux, Windows e OS X)

No Linux e Windows, a tecla `Command` não tem algum efeito,
então use `CommandOrControl` que representa `Command` no OS X e
`Control` no Linux e Window para definir alguns aceleradores.

A chave `Super` está mapeada para `WIndows` e Linux e `Cmd` no OS X.

## Modificadores disponiveis

* `Command` (ou `Cmd` abreviado)
* `Control` (ou `Ctrl` abreviado)
* `CommandOrControl` (ou `CmdOrCtrl` abreviado)
* `Alt`
* `Shift`
* `Super`

## Códigos chaves disponiveis

* `0` até `9`
* `A` até `Z`
* `F1` até `F24`
* Pontuações como`~`, `!`, `@`, `#`, `$`, etc.
* `+`
* `Espaço`
* `Backspace`
* `Delete`
* `Insert`
* `Return` (or `Enter` as alias)
* `Seta para cima (Up)`, `Seta para baixo (Down)`, `seta para esquerda (Left)` e `seta para direita (Right)`
* `Home` and `End`
* `PageUp` and `PageDown`
* `Escape` (or `Esc` abreviado)
* `Aumentar Volume (VolumeUp)`, `Abaixar Volume (VolumeDown)` e `Deixar em Mute o Volume (VolumeMute)`
* `Próxima Música (MediaNextTrack)`, `Música Anterior (MediaPreviousTrack)`, `Parar a Musica (MediaStop)` e `Iniciar ou parar a Musica (MediaPlayPause)`

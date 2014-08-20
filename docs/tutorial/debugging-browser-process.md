# Debugging browser process

The devtools of browser window can only debug the scripts of the web pages
(e.g. the renderer process), in order to provide a way to debug the scripts of
the browser side (e.g. the browser process), atom-shell has provided the
`--debug` and `--debug-brk` switches.

## `--debug=[port]`

When this switch is used atom-shell would listen for V8 debugger protocol on
`port`, the `port` is `5858` by default.

## `debug-brk=[port]`

Like `--debug` but pauses the script on the first line.

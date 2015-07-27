# Depurando el proceso principal

Los devtools sólo pueden depurar las páginas web (el código del proceso renderer).
Para depurar el código del proceso principal, Electron provee dos opciones para la línea de comandos: `--debug` y `--debug-brk`.

## Opciones para la línea de comandos

### `--debug=[port]`

Esta opción escuchará mensajes del protocolo de depuración V8 en `port`, por defecto `port` es `5858`.

### `--debug-brk=[port]`

Similar a `--debug` pero realiza una pausa en la primera línea del script.

## Utilizando node-inspector para depuración

__Nota:__ Electron utiliza node v0.11.13, esta versión aún no funciona bien con node-inspector,
el proceso principal podría fallar al inspeccionar el objeto `process`.

### 1. Iniciar [node-inspector][node-inspector]

```bash
$ node-inspector
```

### 2. Activar el modo de depuración en Electron

Es posible iniciar Electron con la opción de depuración:

```bash
$ electron --debug=5858 your/app
```

o, pausar el script en la primera línea:

```bash
$ electron --debug-brk=5858 your/app
```

### 3. Cargar la interfaz del depurador

Abre http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 en Chrome.

[node-inspector]: https://github.com/node-inspector/node-inspector

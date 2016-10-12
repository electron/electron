# Parámetros CLI soportados (Chrome)

Esta página lista las líneas de comandos usadas por el navegador Chrome que también son
soportadas por Electron. Puedes usar [app.commandLine.appendSwitch][append-switch] para
anexarlas en el script principal de tu aplicación antes de que el evento [ready][ready] del
módulo [app][app] sea emitido:

```javascript
var app = require('app')
app.commandLine.appendSwitch('remote-debugging-port', '8315')
app.commandLine.appendSwitch('host-rules', 'MAP * 127.0.0.1')

app.on('ready', function () {
  // Your code here
})
```

## --client-certificate=`path`

Establece el `path` del archivo de certificado del cliente.

## --ignore-connections-limit=`domains`

Ignora el límite de conexiones para la lista de `domains` separados por `,`.

## --disable-http-cache

Deshabilita la caché del disco para las peticiones HTTP.

## --remote-debugging-port=`port`

Habilita la depuración remota a través de HTTP en el puerto especificado.

## --proxy-server=`address:port`

Usa un servidor proxy especificado, que sobreescribe la configuración del sistema.
Este cambio solo afecta peticiones HTTP y HTTPS.

## --proxy-pac-url=`url`

Utiliza el script PAC en la `url` especificada.

## --no-proxy-server

No usa un servidor proxy y siempre establece conexiones directas. Anula cualquier
otra bandera de servidor proxy bandera que se pase.

## --host-rules=`rules`

Una lista separada por comas de `rules` (reglas) que controlan cómo se asignan los
nombres de host.

Por ejemplo:

* `MAP * 127.0.0.1` Obliga a todos los nombres de host a ser asignados a 127.0.0.1
* `MAP *.google.com proxy` Obliga todos los subdominios google.com a resolverse con
  "proxy".
* `MAP test.com [::1]:77` Obliga a  resolver "test.com" con un bucle invertido de IPv6.
  También obligará a que el puerto de la dirección respuesta sea 77.
* `MAP * baz, EXCLUDE www.google.com` Reasigna todo a "baz", excepto a "www.google.com".

Estas asignaciones especifican el host final en una petición de red (Anfitrión de la conexión TCP
y de resolución de conexión directa, y el `CONNECT` en una conexión proxy HTTP, y el host final de
la conexión proxy `SOCKS`).

## --host-resolver-rules=`rules`

Como `--host-rules` pero estas `rules` solo se aplican al solucionador.

[app]: app.md
[append-switch]: app.md#appcommandlineappendswitchswitch-value
[ready]: app.md#event-ready

## --ignore-certificate-errors

Ignora errores de certificado relacionados.

## --ppapi-flash-path=`path`

Asigna la ruta `path` del pepper flash plugin.

## --ppapi-flash-version=`version`

Asigna la versión `version` del pepper flash plugin.

## --log-net-log=`path`

Permite guardar y escribir eventos de registros de red en `path`.

## --ssl-version-fallback-min=`version`

Establece la versión mínima de SSL/TLS ("tls1", "tls1.1" o "tls1.2") que
el repliegue de TLC aceptará.

## --enable-logging

Imprime el registro de Chromium en consola.

Este cambio no puede ser usado en `app.commandLine.appendSwitch` ya que se analiza antes de que la
aplicación del usuario esté cargada.

## --v=`log_level`

Da el máximo nivel activo de V-logging por defecto; 0 es el predeterminado. Valores positivos
son normalmente usados para los niveles de V-logging.

Este modificador sólo funciona cuando también se pasa `--enable-logging`.

## --vmodule=`pattern`

Da los niveles máximos de V-logging por módulo para sobreescribir el valor dado por
`--v`. Ej. `my_module=2,foo*=3` cambiaría el nivel de registro para todo el código,
los archivos de origen `my_module.*` y `foo*.*`.

Cualquier patrón que contiene un slash o un slash invertido será probado contra toda la ruta
y no sólo con el módulo. Ej. `*/foo/bar/*=2` cambiaría el nivel de registro para todo el código
en los archivos origen bajo un directorio `foo/bar`.

Este modificador sólo funciona cuando también se pasa `--enable-logging`.

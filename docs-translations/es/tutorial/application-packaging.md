# Empaquetamiento de aplicaciones

Para proteger los recursos y el código fuente de tu aplicación, puedes optar por empaquetar
tu aplicación en un paquete [asar][asar].

## Generando un archivo `asar`

Un paquete [asar][asar] es un formato sencillo similar a tar, este formato concatena todos los archivos en uno,
Electron puede leer el contenido sin desempaquetar todos los archivos.

A continuación, los pasos para empaquetar tu aplicación con `asar`:

### 1. Instalar asar

```bash
$ npm install -g asar
```

### 2. Empaquetar utilizando `asar pack`

```bash
$ asar pack your-app app.asar
```

## Utilizando los paquetes `asar`

En Electron existen dos tipos de APIs: las APIs de Node, provistas por Node.js,
y las APIs Web, provistas por Chromium. Ambas APIs soportan la lectura de paquetes `asar`.

### API Node

Con parches especiales en Electron, las APIs de Node como `fs.readFile` and `require`
tratan los paquetes `asar` como directorios virtuales, y el contenido es accesible como si se tratara
de archivos normales en el sistema de archivos.

Por ejemplo, supongamos que tenemos un paquete `example.asar` bajo `/path/to`:

```bash
$ asar list /path/to/example.asar
/app.js
/file.txt
/dir/module.js
/static/index.html
/static/main.css
/static/jquery.min.js
```

Leer un archivo de nuestro paquete `asar`:

```javascript
var fs = require('fs');
fs.readFileSync('/path/to/example.asar/file.txt');
```

Listar todos los archivos de la raíz:

```javascript
var fs = require('fs');
fs.readdirSync('/path/to/example.asar');
```

Utilizar un módulo que se encuentra dentro del archivo:

```javascript
require('/path/to/example.asar/dir/module.js');
```

También puedes mostrar una página web contenida en un `asar` utilizando `BrowserWindow`.

```javascript
var BrowserWindow = require('browser-window');
var win = new BrowserWindow({width: 800, height: 600});
win.loadURL('file:///path/to/example.asar/static/index.html');
```

### API Web

En una págin web, los archivos que se encuentran en el paquete son accesibles a través del protocolo `file:`.
Al igual que la API Node, los paquetes `asar` son tratados como directorios.

Por ejemplo, para obtener un archivo con `$.get`:

```html
<script>
var $ = require('./jquery.min.js');
$.get('file:///path/to/example.asar/file.txt', function(data) {
  console.log(data);
});
</script>
```

### Utilizando un paquete `asar` como un archivo normal

En algunas situaciones necesitaremos acceder al paquete `asar` como archivo, por ejemplo,
si necesitáramos verificar la integridad del archivo con un checksum.
Para casos así es posible utilizar el módulo  `original-fs`, que provee la API `fs` original:

```javascript
var originalFs = require('original-fs');
originalFs.readFileSync('/path/to/example.asar');
```

## Limitaciones de la API Node:

A pesar de que hemos intentado que los paquetes  `asar` funcionen como directorios de la mejor forma posible,
aún existen limitaciones debido a la naturaleza de bajo nivel de la API Node.

### Los paquetes son de sólo lecutra

Los paquetes `asar` no pueden ser modificados, por lo cual todas las funciones que modifiquen archivos
no funcionarán.

## Los directorios del paquete no pueden establecerse como working directories

A pesar de que los paquetes `asar` son manejados virtualmente como directorios,
estos directorios no existen en el sistema de archivos, por lo cual no es posible establecerlos
como working directory, el uso de la opción `cwd` en algunas APIs podría causar errores.

### Desempaquetamiento adicional en algunas APIs

La mayoría de las APIs `fs` pueden leer u obtener información sobre un archivo en un paquete `asar` sin
la necesidad de desempaquetarlo, pero algunas APIs requieren la ruta real. En estos casos Electron extraerá
el archivo a una ruta temporal. Esto agrega un overhead a algunas APIs.

Las APIs que requieren el desempaquetamiento adicional son:

* `child_process.execFile`
* `fs.open`
* `fs.openSync`
* `process.dlopen` - Utilizado por `require` en los módulos nativos

### Información falsa en `fs.stat`

El objeto `Stats` retornado por `fs.stat` y otras funciones relacionadas,
no es preciso, ya que los archivos del paquete `asar` no existen el sistema de archivos.
La utilización del objeto `Stats` sólo es recomendable para obtener el tamaño del archivo y/o
comprobar el tipo de archivo.


## Agregando archivos al paquete `asar`

Como se menciona arriba, algunas APIs de Node desempaquetarán archivos cuando exista una llamada
que los referencie, además de los problemas de rendimiento que esto podría ocasionar, también
podría accionar alertas falsas en software antivirus.

Para lidiar con esto, puedes desempaquetar algunos archivos utilizando la opción `--unpack`.
A continuación, un ejemplo que excluye las librerías compartidas de los módulos nativos:

```bash
$ asar pack app app.asar --unpack *.node
```

Después de ejecutar este comando, además del archivo `app.asar`, también se creará
un directorio `app.asar.unpacked`, que contendrá los archivos desempaquetados.
Este directorio deberá copiarse junto con el archivo `app.asar` a la hora de distribuir la aplicación.

[asar]: https://github.com/atom/asar

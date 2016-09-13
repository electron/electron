# Gúia de estilo de Electron

Encuentra el apartado correcto para cada tarea: [leer la documentación de Electron](#reading-electron-documentation)
o [escribir documentación para Electron](#writing-electron-documentation).

## Escribir Documentación para Electron

Estas son las maneras en las que construimos la documentación de Electron.

- Máximo un título `h1` por página.
- Utilizar `bash` en lugar de `cmd` en los bloques de código (por el resaltado
  de sintaxis).
- Los títulos `h1` en el documento deben corresponder al nombre del objeto
  (ej. `browser-window` → `BrowserWindow`).
  - Archivos separados por guiones, mas sin embargo, es correcto.
- No subtítulos seguidos por otros subtítulos, añadir por lo menos un enunciado
  de descripción.
- Métodos de cabecera son delimitados con apóstrofes: `código`.
- Cabeceras de Eventos son delimitados con 'comillas' simples.
- No generar listas de mas de dos niveles (debido al renderizador de Markdown
  desafortunadamente).
- Agregar títulos de sección: Eventos, Métodos de Clases y Métodos de Instancia.
- Utilizar 'deberá' en lugar de 'debería' al describir resultados.
- Eventos y Métodos son cabeceras `h3`.
- Argumentos opcionales escritos como `function (required[, optional])`.
- Argumentos opcionales son denotados cuando se llaman en listas.
- Delimitador de línea de 80-columnas.
- Métodos específicos de Plataformas son denotados en itálicas seguidas por la cabecera del método.
  - ```### `method(foo, bar)` _macOS_```
- Preferir 'en el ___ proceso' en lugar de 'sobre el'

### Traducciones de la Documentación

Traducciones de documentos de Electron se encuentran dentro del folder
`docs-translations`.

Para agregar otro set (o un set parcial):

- Crear un subdirectorio nombrado igual a la abreviación del lenguaje.
- Dentro de ese subdirectorio, duplicar el directorio de `docs`, manteniendo los
  mismos nombres de directorios y archivos.
- Traducir los archivos.
- Actualizar el `README.md` dentro del subdirectorio del lenguaje apuntando a
  los archivos que has traducido.
- Agregar un enlace al folder de tu traducción en la sección principal Electron
[README](https://github.com/electron/electron#documentation-translations).

## Leyendo la Documentación de Electron

Estos son algunos consejos para entender la sintaxis de la documentación de
Electron.

### Métodos

Un ejemplo de la documentación del [método](https://developer.mozilla.org/en-US/docs/Glossary/Method):

---

`methodName(required[, optional]))`

* `require` String, **required**
* `optional` Integer

---

El nombre del método es seguido por los argumentos que recibe. Argumentos
opcionales son denotados por corchetes rodeados por el argumento opcional y la
coma requerida si el argumento opcional fuera seguido por otro argumento.

Debajo del método se encuentra más información detallada de cada uno de los
argumentos. El tipo de argumento es denotado por los tipos comúnes:
[`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String),
[`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number),
[`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object),
[`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
o un tipo personalizado como el [`webContent`](api/web-content.md) de Electron.

### Eventos

Un ejemplo de documentación del [evento](https://developer.mozilla.org/en-US/docs/Web/API/Event):

---

Event: 'wake-up'

Returns:

* `time` String

---

El evento es una cadena que es utilizada luego de un método observador `.on`. Si
regresa un valor, él y su tipo son denotados abajo. Si se estaba a la escucha y
respondió a este evento se debería ver así:

```javascript
Alarm.on('wake-up', function(time) {
  console.log(time)
})
```

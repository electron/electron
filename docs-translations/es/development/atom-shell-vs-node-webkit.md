#Diferencias Técnicas entre Electron y  NW.js (anteriormente conocido como node-webkit)

**Nota:Electron se llamaba antes Atom Shell.**

Como NW.js, Electron proporciona una plataforma para escribir aplicaciones de escritorio con JavaScript y HTML y tiene la integración de nodo para permitir el acceso al sistema de bajo nivel de las páginas web.

Pero también hay diferencias fundamentales entre los dos proyectos que hacen a Electron un producto totalmente independiente de NW.js:

**1. Ingreso a la aplicación**

En NW.js el principal punto de ingreso de una aplicación es una página web. Usted especifica una página principal de URL en el `package.json` y se abre en una ventana del navegador como ventana principal de la aplicación.

En Electron, el punto de ingreso es un script de JavaScript. En lugar de proporcionar una dirección URL directamente, usted crea manualmente una ventana del navegador y carga un archivo HTML utilizando la API. También es necesario escuchar a los eventos de la ventana para decidir cuándo salir de la aplicación.

Electron funciona más como el tiempo de ejecución(Runtime) de Node.js. Las Api's de Electron son de bajo nivel asi que  puede usarlo para las pruebas del navegador en lugar de  usar [PhantomJS.](http://phantomjs.org/)

**2.Construir un sistema**

Con el fin de evitar la complejidad de la construcción de todo Chromium, Electron utiliza `libchromiumcontent` para acceder a al contenido  Chromium's API. `libchromiumcontent` es solo una liberia compartida que incluye el módulo de contenido de Chromium y todas sus dependencias. Los usuarios no necesitan una máquina potente para construir con Electron.

**3.Integración de Node**

In NW.js, the Node integration in web pages requires patching Chromium to work, while in Electron we chose a different way to integrate the libuv loop with each platform's message loop to avoid hacking Chromium. See the node_bindings code for how that was done.

En NW.js, la integración de Node en las páginas web requiere parchear Chromium para que funcione, mientras que en Electron elegimos una manera diferente para integrar el cilco libuv con cada ciclo de mensaje de las plataformas para evitar el hacking en Chromium. Ver el código [`node_bindings`][node-bindings] de cómo se hizo.


**4. Multi-contexto**

Si usted es un usuario experimentado NW.js, usted debe estar familiarizado con el concepto de contexto Node y el contexto web. Estos conceptos fueron inventados debido a la forma cómo se implementó NW.js.

Mediante el uso de la característica [multi-contexto](http://strongloop.com/strongblog/whats-new-node-js-v0-12-multiple-context-execution/) de Node, Electron no introduce un nuevo contexto JavaScript en páginas web.Resultados de búsqueda

[node-bindings]: https://github.com/electron/electron/tree/master/atom/common

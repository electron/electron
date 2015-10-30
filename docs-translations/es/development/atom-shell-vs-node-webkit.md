#Diferencias Técnicas entre Electron y  NW.js (anteriormente conocido como node-webkit)

**Nota:Electron se llamaba antes Atom Shell.**

Como NW.js, Electron proporciona una plataforma para escribir aplicaciones de escritorio con JavaScript y HTML y tiene la integración de nodo para permitir el acceso al sistema de bajo nivel de las páginas web.

Pero también hay diferencias fundamentales entre los dos proyectos que hacen a Electron un producto totalmente independiente de NW.js:

**1. Ingreso a la aplicación**

En NW.js el principal punto de ingreso de una aplicación es una página web. Usted especifica una página principal de URL en el package.json y se abre en una ventana del navegador como ventana principal de la aplicación.

En Electron, el punto de ingreso es un script de JavaScript. En lugar de proporcionar una dirección URL directamente, usted crea manualmente una ventana del navegador y carga un archivo HTML utilizando la API. También es necesario escuchar a los eventos de la ventana para decidir cuándo salir de la aplicación.

Electron works more like the Node.js runtime. Electron's APIs are lower level so you can use it for browser testing in place of [PhantomJS.](http://phantomjs.org/)

2. Build System

In order to avoid the complexity of building all of Chromium, Electron uses libchromiumcontent to access Chromium's Content API. libchromiumcontent is a single shared library that includes the Chromium Content module and all of its dependencies. Users don't need a powerful machine to build Electron.

3. Node Integration

In NW.js, the Node integration in web pages requires patching Chromium to work, while in Electron we chose a different way to integrate the libuv loop with each platform's message loop to avoid hacking Chromium. See the node_bindings code for how that was done.

4. Multi-context

If you are an experienced NW.js user, you should be familiar with the concept of Node context and web context. These concepts were invented because of how NW.js was implemented.

By using the multi-context feature of Node, Electron doesn't introduce a new JavaScript context in web pages.

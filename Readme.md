Logotipo de Electron
Estado de la compilación de CircleCI Estado de compilación de AppVeyor Invitación de discordia de electrones

📝 Traducciones disponibles: 🇨🇳 🇧🇷 🇪🇸 🇯🇵 🇷🇺 🇫🇷 🇺🇸 🇩🇪. Vea estos documentos en otros idiomas en electron / i18n .

El marco de Electron le permite escribir aplicaciones de escritorio multiplataforma utilizando JavaScript, HTML y CSS. Está basado en Node.js y Chromium y es utilizado por el editor Atom y muchas otras aplicaciones .

Siga a @ElectronJS en Twitter para recibir anuncios importantes.

Este proyecto se adhiere al código de conducta del Pacto de Colaboradores . Al participar, se espera que respete este código. Informe el comportamiento inaceptable a coc@electronjs.org .

Instalación
Para instalar archivos binarios de Electron prediseñados, utilice npm. El método preferido es instalar Electron como una dependencia de desarrollo en su aplicación:

npm install electron --save-dev
Para obtener más opciones de instalación y sugerencias para la resolución de problemas, consulte instalación . Para obtener información sobre cómo administrar las versiones de Electron en sus aplicaciones, consulte Control de versiones de Electron .

Inicio rápido y Electron Fiddle
Úselo Electron Fiddle para construir, ejecutar y empaquetar pequeños experimentos de Electron, para ver ejemplos de código para todas las API de Electron y para probar diferentes versiones de Electron. Está diseñado para facilitar el inicio de su viaje con Electron.

Alternativamente, clone y ejecute el repositorio de inicio rápido de electrones / electrones para ver una aplicación mínima de Electron en acción:

clon de git https://github.com/electron/electron-quick-start
 cd electron-quick-start
npm install
inicio npm
Recursos para aprender Electron
electronjs.org/docs : toda la documentación de Electron
electron / fiddle : una herramienta para construir, ejecutar y empaquetar pequeños experimentos de electrones
electron / electron-quick-start : una aplicación básica muy básica de Electron
electronjs.org/community#boilerplates : aplicaciones de inicio de muestra creadas por la comunidad
electron / simple-samples : pequeñas aplicaciones con ideas para llevarlas más lejos
electron / electron-api-demos : una aplicación de Electron que le enseña a usar Electron
Uso programático
La mayoría de las personas usan Electron desde la línea de comandos, pero si lo necesita electrondentro de su aplicación Node (no su aplicación Electron), devolverá la ruta del archivo al binario. Use esto para generar Electron a partir de scripts de nodo:

const  electron  =  require ( 'electron' ) 
const  proc  =  require ( 'child_process' )

// imprimirá algo similar a /Users/maf/.../Electron 
console . log ( electrón )

// generar Electron 
const  child  =  proc . spawn ( electrón )
Espejos
porcelana
Traducciones de documentación
Encuentre traducciones de documentación en electron / i18n .

Contribuyendo
Si está interesado en informar / solucionar problemas y contribuir directamente a la base del código, consulte CONTRIBUTING.md para obtener más información sobre lo que estamos buscando y cómo comenzar.

Comunidad
En el documento de soporte se puede encontrar información sobre cómo informar errores, obtener ayuda, encontrar herramientas de terceros y aplicaciones de muestra, etc.

Licencia
MIT

Al usar Electron u otros logotipos de GitHub, asegúrese de seguir las pautas del logotipo de GitHub .

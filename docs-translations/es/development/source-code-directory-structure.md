# Estructura de los directorios del código fuente

El código fuente de electron es separado en pocas partes, en su mayoría
siguiendo las especificaciones para separar archivos que usa Chromium.

Quizá necesites familiarizarte con la [arquitectura multiprocesos](http://dev.chromium.org/developers/design-documents/multi-process-architecture) de Chromium para comprender mejor el código fuente.

## Estructura del código fuente

```
Electron
├──atom - Código fuente de Electron.
|  ├── app - Código de arranque.
|  ├── browser - La interfaz incluyendo la ventana principal, UI,
|  |   y todas las cosas del proceso principal. Este le habla al renderizador
|  |   para manejar las páginas web.
|  |   ├── lib - Código Javascript para inicializar el proceso principal.
|  |   ├── ui - Implementaciones de UI para distintas plataformas.
|  |   |   ├── cocoa - Código fuente específico para Cocoa.
|  |   |   ├── gtk - Código fuente específico para GTK+.
|  |   |   └── win - Código fuente específico para Windows GUI.
|  |   ├── default_app - La página por defecto para mostrar cuando Electron
|  |   |   es iniciado sin proveer una app.
|  |   ├── api - La implementación de las APIs para el proceso principal.
|  |   |   └── lib - Código Javascript parte de la implementación de la API.
|  |   ├── net - Código relacionado a la red.
|  |   ├── mac - Código fuente de Objective-C específico para Mac.
|  |   └── resources - Iconos, archivos específicos de plataforma, etc.
|  ├── renderer - Código que se ejecuta en el proceso de renderizado.
|  |   ├── lib - Código Javascript del proceso de inicio del renderizador.
|  |   └── api - La implementación de las APIs para el proceso de renderizado.
|  |       └── lib - Código Javascript parte de la implementación de la API.
|  └── common - Código que se utiliza en ambos procesos, el principal y el de
|      renderizado. Incluye algunas funciones de utilidad y código para integrar
|      el ciclo de mensajes de Node en el ciclo de mensajes de Chromium.
|      ├── lib - Código Javascript común para la inicialización.
|      └── api - La implementación de APIs comunes, y los fundamentos de
|          los módulos integrados de Electron.
|          └── lib - Código Javascript parte de la implementación de la API.
├── chromium_src - Código fuente copiado de Chromium.
├── docs - Documentación.
├── spec - Pruebas automaticas.
├── atom.gyp - Reglas de compilado de Electron.
└── common.gypi - Configuración específica para compilar y reglas
    de empaquetado para otros componentes como `node` y `breakpad`.
```

## Estructura de otros directorios

* **script** - Scripts usados para propositos de desarrollo
  como compilar, empaquetar, realizar pruebas, etc.
* **tools** - Scripts de ayuda usados por los archivos gyp, contrario a la
  carpeta `scripts`, estos scripts nunca deberían ser llamados por los usuarios.
* **vendor** - Código fuente de dependencias externas, no usamos `third_party`
  como nombre por que se podría confundir con el mismo directorio
  en las carpetas del código fuente de Chromium.
* **node_modules** - Módulos de node usados para la compilación.
* **out** - Directorio temporal de salida usado por `ninja`.
* **dist** - Directorio temporal creado por `script/create-dist.py` cuando
  se esta creando una distribución.
* **external_binaries** - Binarios descargados de frameworks externos que no
  soportan la compilación con `gyp`.

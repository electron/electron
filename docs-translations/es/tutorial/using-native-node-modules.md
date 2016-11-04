# Utilizando módulos Node nativos

Los módulos Node nativos son soportados por Electron, pero dado que Electron
está utilizando una versión distinta de V8, debes especificar manualmente la
ubicación de las cabeceras de Electron a la hora de compilar módulos nativos.

## Compatibilidad de módulos nativos

A partir de Node v0.11.x han habido cambios vitales en la API de V8.
Es de esperar que los módulos escritos para Node v0.10.x no funcionen con Node v0.11.x.
Electron utiliza Node v.0.11.13 internamente, y por este motivo tiene el mismo problema.

Para resolver esto, debes usar módulos que soporten Node v0.11.x,
[muchos módulos](https://www.npmjs.org/browse/depended/nan) soportan ambas versiones.
En el caso de los módulos antiguos que sólo soportan Node v0.10.x, debes usar el módulo
[nan](https://github.com/rvagg/nan) para portarlos a v0.11.x.

## ¿Cómo instalar módulos nativos?

### La forma fácil

La forma más sencilla de recompilar módulos nativos es a través del paquete
[`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild),
el cual abstrae y maneja los pasos de descargar las cabeceras y compilar los módulos nativos:

```sh
npm install --save-dev electron-rebuild

# Ejecuta esto cada vez que ejecutes npm install
./node_modules/.bin/electron-rebuild
```

### La forma node-gyp

Para compilar módulos Node con las cabeceras de Electron, debes indicar a `node-gyp`
desde dónde descargar las cabeceras y cuál versión usar:

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/electron
```

Los cambios en `HOME=~/.electron-gyp` fueron para especificar la ruta de las cabeceras.
La opción `--target=0.29.1` es la versión de Electron. La opción `--dist-url=...` especifica
dónde descargar las cabeceras. `--arch=x64` indica que el módulo será compilado para un sistema de 64bit.

### La forma npm

También puedes usar `npm` para instalar módulos, los pasos son exactamente igual a otros módulos Node,
con la excepción de que necesitas establecer algunas variables de entorno primero:

```bash
export npm_config_disturl=https://atom.io/download/electron
export npm_config_target=0.29.1
export npm_config_arch=x64
HOME=~/.electron-gyp npm install module-name
```

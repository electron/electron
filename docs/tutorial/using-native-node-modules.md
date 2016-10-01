# Using Native Node Modules

The native Node modules are supported by Electron, but since Electron is very
likely to use a different V8 version from the Node binary installed in your
system, you have to manually specify the location of Electron's headers when
building native modules.

## How to install native modules

Three ways to install native modules:

### Using `npm`

By setting a few environment variables, you can use `npm` to install modules
directly.

An example of installing all dependencies for Electron:

```bash
# Electron's version.
export npm_config_target=1.2.3
# The architecture of Electron, can be ia32 or x64.
export npm_config_arch=x64
export npm_config_target_arch=x64
# Download headers for Electron.
export npm_config_disturl=https://atom.io/download/atom-shell
# Tell node-pre-gyp that we are building for Electron.
export npm_config_runtime=electron
# Tell node-pre-gyp to build module from source code.
export npm_config_build_from_source=true
# Install all dependencies, and store cache to ~/.electron-gyp.
HOME=~/.electron-gyp npm install
```

### Installing modules and rebuilding for Electron

You can also choose to install modules like other Node projects, and then
rebuild the modules for Electron with the [`electron-rebuild`][electron-rebuild]
package. This module can get the version of Electron and handle the manual steps
of downloading headers and building native modules for your app.

An example of installing `electron-rebuild` and then rebuild modules with it:

```bash
npm install --save-dev electron-rebuild

# Every time you run "npm install", run this:
./node_modules/.bin/electron-rebuild

# On Windows if you have trouble, try:
.\node_modules\.bin\electron-rebuild.cmd
```

### Manually building for Electron

If you are a developer developing a native module and want to test it against
Electron, you might want to rebuild the module for Electron manually. You can
use `node-gyp` directly to build for Electron:

```bash
cd /path-to-module/
HOME=~/.electron-gyp node-gyp rebuild --target=1.2.3 --arch=x64 --dist-url=https://atom.io/download/atom-shell
```

The `HOME=~/.electron-gyp` changes where to find development headers. The
`--target=1.2.3` is version of Electron. The `--dist-url=...` specifies
where to download the headers. The `--arch=x64` says the module is built for
64bit system.

## Troubleshooting

If you installed a native module and found it was not working, you need to check
following things:

* The architecture of module has to match Electron's architecture (ia32 or x64).
* After you upgraded Electron, you usually need to rebuild the modules.
* When in doubt, run `electron-rebuild` first.

## Modules that rely on `node-pre-gyp`

The [`node-pre-gyp` tool][node-pre-gyp] provides a way to deploy native Node
modules with prebuilt binaries, and many popular modules are using it.

Usually those modules work fine under Electron, but sometimes when Electron uses
a newer version of V8 than Node, and there are ABI changes, bad things may
happen. So in general it is recommended to always build native modules from
source code.

If you are following the `npm` way of installing modules, then this is done
by default, if not, you have to pass `--build-from-source` to `npm`, or set the
`npm_config_build_from_source` environment variable.

[electron-rebuild]: https://github.com/paulcbetts/electron-rebuild
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp

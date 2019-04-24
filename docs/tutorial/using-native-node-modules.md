# Using Native Node Modules

Native Node modules are supported by Electron, but since Electron is very
likely to use a different V8 version from the Node binary installed on your
system, the modules you use will need to be recompiled for Electron. Otherwise,
you will get the following class of error when you try to run your app:

```sh
Error: The module '/path/to/native/module.node'
was compiled against a different Node.js version using
NODE_MODULE_VERSION $XYZ. This version of Node.js requires
NODE_MODULE_VERSION $ABC. Please try re-compiling or re-installing
the module (for instance, using `npm rebuild` or `npm install`).
```

## How to install native modules

There are several different ways to install native modules:

### Installing modules and rebuilding for Electron

You can install modules like other Node projects, and then rebuild the modules
for Electron with the [`electron-rebuild`][electron-rebuild] package. This
module can automatically determine the version of Electron and handle the
manual steps of downloading headers and rebuilding native modules for your app.

For example, to install `electron-rebuild` and then rebuild modules with it
via the command line:

```sh
npm install --save-dev electron-rebuild

# Every time you run "npm install", run this:
./node_modules/.bin/electron-rebuild

# On Windows if you have trouble, try:
.\node_modules\.bin\electron-rebuild.cmd
```

For more information on usage and integration with other tools, consult the
project's README.

### Using `npm`

By setting a few environment variables, you can use `npm` to install modules
directly.

For example, to install all dependencies for Electron:

```sh
# Electron's version.
export npm_config_target=1.2.3
# The architecture of Electron, see https://electronjs.org/docs/tutorial/support#supported-platforms
# for supported architectures.
export npm_config_arch=x64
export npm_config_target_arch=x64
# Download headers for Electron.
export npm_config_disturl=https://atom.io/download/electron
# Tell node-pre-gyp that we are building for Electron.
export npm_config_runtime=electron
# Tell node-pre-gyp to build module from source code.
export npm_config_build_from_source=true
# Install all dependencies, and store cache to ~/.electron-gyp.
HOME=~/.electron-gyp npm install
```

### Manually building for Electron

If you are a developer developing a native module and want to test it against
Electron, you might want to rebuild the module for Electron manually. You can
use `node-gyp` directly to build for Electron:

```sh
cd /path-to-module/
HOME=~/.electron-gyp node-gyp rebuild --target=1.2.3 --arch=x64 --dist-url=https://atom.io/download/electron
```

* `HOME=~/.electron-gyp` changes where to find development headers.
* `--target=1.2.3` is the version of Electron.
* `--dist-url=...` specifies where to download the headers.
* `--arch=x64` says the module is built for a 64-bit system.

### Manually building for a custom build of Electron

To compile native Node modules against a custom build of Electron that doesn't
match a public release, instruct `npm` to use the version of Node you have bundled
with your custom build.

```sh
npm rebuild --nodedir=/path/to/electron/vendor/node
```

## Troubleshooting

If you installed a native module and found it was not working, you need to check
the following things:

* When in doubt, run `electron-rebuild` first.
* Make sure the native module is compatible with the target platform and
  architecture for your Electron app.
* Make sure `win_delay_load_hook` is not set to `false` in the module's `binding.gyp`.
* After you upgrade Electron, you usually need to rebuild the modules.

### A note about `win_delay_load_hook`

On Windows, by default, `node-gyp` links native modules against `node.dll`.
However, in Electron 4.x and higher, the symbols needed by native modules are
exported by `electron.exe`, and there is no `node.dll`. In order to load native
modules on Windows, `node-gyp` installs a [delay-load
hook](https://msdn.microsoft.com/en-us/library/z9h1h6ty.aspx) that triggers
when the native module is loaded, and redirects the `node.dll` reference to use
the loading executable instead of looking for `node.dll` in the library search
path (which would turn up nothing). As such, on Electron 4.x and higher,
`'win_delay_load_hook': 'true'` is required to load native modules.

## Modules that rely on `prebuild`

[`prebuild`](https://github.com/prebuild/prebuild) provides a way to publish
native Node modules with prebuilt binaries for multiple versions of Node
and Electron.

If modules provide binaries for the usage in Electron, make sure to omit
`--build-from-source` and the `npm_config_build_from_source` environment
variable in order to take full advantage of the prebuilt binaries.

## Modules that rely on `node-pre-gyp`

The [`node-pre-gyp` tool][node-pre-gyp] provides a way to deploy native Node
modules with prebuilt binaries, and many popular modules are using it.

Usually those modules work fine under Electron, but sometimes when Electron uses
a newer version of V8 than Node and/or there are ABI changes, bad things may
happen. So in general, it is recommended to always build native modules from
source code. `electron-rebuild` handles this for you automatically.

If you are following the `npm` way of installing modules, then this is done
by default, if not, you have to pass `--build-from-source` to `npm`, or set the
`npm_config_build_from_source` environment variable.

[electron-rebuild]: https://github.com/electron/electron-rebuild
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp

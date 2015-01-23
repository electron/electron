# Using native Node modules

The native Node modules are supported by atom-shell, but since atom-shell is
using a different V8 version from official Node, you need to use `apm` instead
of `npm` to install Node modules.

The usage of [apm](https://github.com/atom/apm) is quite similar to `npm`, to
install dependencies from `package.json` of current project, just do:

```bash
$ cd /path/to/atom-shell/project/
$ apm install .
```

But you should notice that `apm install module` won't work because it will
install a user package for [Atom Editor](https://github.com/atom/atom) instead.

## Which version of apm to use

Generally using the latest release of `apm` for latest atom-shell always works,
but if you are uncertain of the which version of `apm` to use, you may manually
instruct `apm` to use headers of a specified version of atom-shell by setting
the `ATOM_NODE_VERSION` environment.

For example force installing modules for atom-shell v0.16.0:

```bash
$ export ATOM_NODE_VERSION=0.16.0
$ apm install .
```

## Native Node module compatibility

Since Node v0.11.x there were vital changes in the V8 API. So generally all
native modules written for Node v0.10.x wouldn't work for Node v0.11.x. And
because atom-shell internally uses Node v0.11.13, it carries with the same
problem.

To solve this, you should use modules that support Node v0.11.x,
[many modules](https://www.npmjs.org/browse/depended/nan) do support both now.
For old modules that only support Node v0.10.x, you should use the
[nan](https://github.com/rvagg/nan) module to port it to v0.11.x.

## Other ways of installing native modules

Apart from `apm`, you can also use `node-gyp` and `npm` to manually build the
native modules.

### The node-gyp way

To build Node modules with headers of atom-shell, you need to tell `node-gyp`
where to download headers and which version to use:

```bash
$ cd /path-to-module/
$ HOME=~/.atom-shell-gyp node-gyp rebuild --target=0.16.0 --arch=ia32 --dist-url=https://atom.io/download/atom-shell
```

The `HOME=~/.atom-shell-gyp` changes where to find development headers. The
`--target=0.16.0` is version of atom-shell. The `--dist-url=...` specifies
where to download the headers. The `--arch=ia32` says the module is built for
32bit system.

### The npm way

```bash
export npm_config_disturl=https://atom.io/download/atom-shell
export npm_config_target=0.6.0
export npm_config_arch=ia32
HOME=~/.atom-shell-gyp npm install module-name
```

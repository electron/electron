# Use native Node modules

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

## Native Node module compatibility

Since Node v0.11.x there were vital changes in the V8 API. So generally all native
modules written for Node v0.10.x wouldn't work for Node v0.11.x. Additionally
since atom-shell internally uses Node v0.11.9, it carries with the same problem.

To solve this, you should use modules that support both Node v0.10.x and v0.11.x.
[Many modules](https://www.npmjs.org/browse/depended/nan) do support both now.
For old modules that only support Node v0.10.x, you should use the
[nan](https://github.com/rvagg/nan) module to port it to v0.11.x.

## Other ways of installing native modules

Apart from `apm`, you can also use `node-gyp` and `npm` to manually build the
native modules.

### The node-gyp way

First you need to check which Node release atom-shell is carrying via
`process.version` (at the time of writing it is v0.10.5). Then you can
configure and build native modules via following commands:

```bash
$ cd /path-to-module/
$ HOME=~/.atom-shell-gyp node-gyp rebuild --target=0.10.5 --arch=ia32 --dist-url=https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist
```

The `HOME=~/.atom-shell-gyp` changes where to find development headers. The
`--target=0.10.5` is specifying Node's version. The `--dist-url=...` specifies
where to download the headers.

### The npm way

```bash
export npm_config_disturl=https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist
export npm_config_target=0.10.5
export npm_config_arch=ia32
HOME=~/.atom-shell-gyp npm install module-name
```

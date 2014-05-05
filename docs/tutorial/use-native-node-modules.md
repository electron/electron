# Use native node modules

The native node modules are supported by atom-shell, but since atom-shell is
using a different V8 version from official node, you need to use `apm` instead
of `npm` to install node modules.

The usage of [apm](https://github.com/atom/apm) is quite similar to `npm`, to
install dependencies from `package.json` of current project, just do:

```bash
$ cd /path/to/atom-shell/project/
$ apm install .
```

But you should notice that `apm install module` wont' work because it will
install a user package for [Atom Editor](https://github.com/atom/atom) instead.

## Native node module compability

Since node v0.11.x, there were vital changes of V8 API, so generally all native
modules written for node v0.10.x wouldn't work for node v0.11.x, and since
atom-shell internally uses node v0.11.9, it carries with the same problem.

To solve it, you should use modules that support both node v0.10.x and v0.11.x,
and [many modules](https://www.npmjs.org/browse/depended/nan) do support the
both now. For old modules that only support node v0.10.x, you should use the
[nan](https://github.com/rvagg/nan) module to port it to v0.11.x.

## Other ways of installing native modules

Apart from `apm`, you can also use `node-gyp` and `npm` to manually build the
native modules.

### The node-gyp way

First you need to check which node release atom-shell is carrying via
`process.version` (at the time of writing it is v0.10.5), then you can
configure and build native modules via following commands:

```bash
$ cd /path-to-module/
$ HOME=~/.atom-shell-gyp node-gyp rebuild --target=0.10.5 --arch=ia32 --dist-url=https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist
```

The `HOME=~/.atom-shell-gyp` changes where to find development headers. The
`--target=0.10.5` is specifying node's version. The `--dist-url=...` specifies
where to download the headers.

### The npm way

```bash
export npm_config_disturl=https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist
export npm_config_target=0.10.5
export npm_config_arch=ia32
HOME=~/.atom-shell-gyp npm install module-name
```

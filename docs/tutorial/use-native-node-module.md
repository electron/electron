# Use native node modules

Since atom-shell is using a different V8 version from the official node, you
need to build native module against atom-shell's headers to use them.

The [apm](https://github.com/atom/apm) provided a easy way to do this, after
installing it you could use it to install dependencies just like using `npm`:

```bash
$ cd /path/to/atom-shell/project/
$ apm install .
```

But you should notice that `apm install module` wont' work because it will
install a user package for [Atom](https://github.com/atom/atom) instead.

Apart from `apm`, you can also use `node-gyp` and `npm` to manually build the
native modules.

## The node-gyp way

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

## The npm way

```bash
export npm_config_disturl=https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist
export npm_config_target=0.10.5
export npm_config_arch=ia32
HOME=~/.atom-shell-gyp npm install module-name
```

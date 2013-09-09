# Build native modules

Since atom-shell is using a different V8 version from the official node, you
need to build native module against atom-shell's headers to use them.

You need to use node-gyp to compile native modules, you can install node-gyp
via npm if you hadn't:

```bash
$ npm install -g node-gyp
```

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

## Use npm to build native modules

Under most circumstances you would want to use npm to install modules, if
you're using npm >= v1.2.19 (because [a
patch](https://github.com/TooTallNate/node-gyp/commit/afbcdea1ffd25c02bc88d119b10337852c44d400)
is needed to make `npm_config_disturl` work) you can use following code to
download and build native modules against atom-shell's headers:

```bash
export npm_config_disturl=https://gh-contractor-zcbenz.s3.amazonaws.com/atom-shell/dist
export npm_config_target=0.10.5
export npm_config_arch=ia32
HOME=~/.atom-shell-gyp npm install module-name
```

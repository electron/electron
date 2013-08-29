## Prerequisites

* Mac OS X >= 10.7
* [Xcode](https://developer.apple.com/technologies/tools/)
* [node.js](http://nodejs.org)

## Getting the code

```bash
$ git clone https://github.com/atom/atom-shell.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
build project files. Notice that we're using `ninja` to build `atom-shell` so
there is no Xcode project generated.

```bash
$ cd atom-shell
$ ./script/bootstrap.py
```

## Building

Build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

You can also only build the `Debug` target:

```bash
$ ./script/build.py -c Debug
```

After building is done, you can find `Atom.app` under `out/Debug`.

## Tests

```bash
$ ./script/test.py
```

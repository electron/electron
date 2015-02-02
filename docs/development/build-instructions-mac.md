# Build instructions (Mac)

## Prerequisites

* OS X >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (external).

If you are using the python downloaded by Homebrew, you also need to install
following python modules:

* pyobjc

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
$ ./script/bootstrap.py -v
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

## 32bit support

Currently atom-shell can only be built for 64bit target on OS X, and there is no
plan to support 32bit on OS X in future.

## Tests

```bash
$ ./script/test.py
```

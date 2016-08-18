# Build Instructions (OS X)

Follow the guidelines below for building Electron on OS X.

## Prerequisites

* OS X >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (external)

If you are using the Python downloaded by Homebrew, you also need to install
following python modules:

* pyobjc

## Getting the Code

```bash
$ git clone https://github.com/atom/electron.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
the build project files. Notice that we're using `ninja` to build Electron so
there is no Xcode project generated.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## Building

Build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

You can also only build the `Debug` target:

```bash
$ ./script/build.py -c D
```

After building is done, you can find `Electron.app` under `out/D`.

## 32bit Support

Electron can only be built for a 64bit target on OS X and there is no plan to
support 32bit OS X in the future.

## Tests

Test your changes conform to the project coding style using:

```bash
$ ./script/cpplint.py
```

Test functionality using:

```bash
$ ./script/test.py
```

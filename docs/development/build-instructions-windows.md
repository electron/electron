# Build instructions (Windows)

## Prerequisites

* Windows 7 or later
* Visual Studio 2010 Express or Profissional
  * Make sure "X64 Compilers and Tools" are installed if you use the
    Profissional edition
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [node.js](http://nodejs.org/)
* [git](http://git-scm.com)

If you are using Visual Studio 2010 __Express__ then you also need following
softwares:

* [WDK](http://www.microsoft.com/en-us/download/details.aspx?id=11800)
* [Windows 7 SDK](http://www.microsoft.com/en-us/download/details.aspx?id=8279)

The instructions bellow are executed under [cygwin](http://www.cygwin.com),
but it's not a requirement, you can also build atom-shell under Windows's
console or other terminals.

The building of atom-shell is done entirely with command line scripts, so you
can use any editor you like to develop atom-shell, but it also means you can
not use Visual Studio for the development. Support of building with Visual
Studio will come in future.

**Note:** Even though Visual Studio is not used for building, it's still
**required** because we need the build toolchains it provided.

## Getting the code

```bash
$ git clone https://github.com/atom/atom-shell.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
build project files. Notice that we're using `ninja` to build atom-shell so
there is no Visual Studio project generated.

```bash
$ cd atom-shell
$ python script/bootstrap.py
```

## Building

Build both Release and Debug targets:

```bash
$ python script/build.py
```

You can also only build the Debug target:

```bash
$ python script/build.py -c Debug
```

After building is done, you can find `atom.exe` under `out\Debug`.

## Tests

```bash
$ python script/test.py
```

# Build instructions (Windows)

## Prerequisites

* Windows 7 or later
* Visual Studio 2010 Express or Professional
  * Make sure "X64 Compilers and Tools" are installed if you use the
    Professional edition.
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [node.js](http://nodejs.org/)
* [git](http://git-scm.com)

If you are using Visual Studio 2010 __Express__ then you also need following
softwares:

* [WDK](http://www.microsoft.com/en-us/download/details.aspx?id=11800)
  * `Build Environments` is required.
* [Windows 7 SDK](http://www.microsoft.com/en-us/download/details.aspx?id=8279)
  * `Windows Headers` and `Visual C++ Compilers` are required.

The instructions below are executed under [cygwin](http://www.cygwin.com),
but it's not a requirement, you can also build atom-shell under the Windows
command prompt or other terminals.

The building of atom-shell is done entirely with command-line scripts, so you
can use any editor you like to develop atom-shell, but it also means you can
not use Visual Studio for the development. Support of building with Visual
Studio will come in the future.

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

## 64bit support

Currently atom-shell can only be built for 32bit target on Windows, support for
64bit will come in future.

## Tests

```bash
$ python script/test.py
```

## Troubleshooting

### Assertion failed: ((handle))->activecnt >= 0

When building under cygwin, you could see `bootstrap.py` failed with following
error:

```
Assertion failed: ((handle))->activecnt >= 0, file src\win\pipe.c, line 1430

Traceback (most recent call last):
  File "script/bootstrap.py", line 87, in <module>
    sys.exit(main())
  File "script/bootstrap.py", line 22, in main
    update_node_modules('.')
  File "script/bootstrap.py", line 56, in update_node_modules
    execute([NPM, 'install'])
  File "/home/zcbenz/codes/raven/script/lib/util.py", line 118, in execute
    raise e
subprocess.CalledProcessError: Command '['npm.cmd', 'install']' returned non-zero exit status 3
```

This is caused by a bug when using cygwin python and win32 node together. The
solution is to use the win32 python to execute the bootstrap script (supposing
you have installed python under `C:\Python27`):

```bash
/cygdrive/c/Python27/python.exe script/bootstrap.py
```

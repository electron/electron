# Build Instructions (Windows)

Follow the guidelines below for building Electron on Windows.

## Prerequisites

* Windows 7 / Server 2008 R2 or higher
* Visual Studio 2015 - [download VS 2015 Community Edition for
  free](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [Node.js](http://nodejs.org/download/)
* [Git](http://git-scm.com)

If you don't currently have a Windows installation,
[dev.microsoftedge.com](https://developer.microsoft.com/en-us/microsoft-edge/tools/vms/)
has timebombed versions of Windows that you can use to build Electron.

Building Electron is done entirely with command-line scripts and cannot be done
with Visual Studio. You can develop Electron with any editor but support for
building with Visual Studio will come in the future.

**Note:** Even though Visual Studio is not used for building, it's still
**required** because we need the build toolchains it provides.

**Note:** While older versions of Electron required Visual Studio 2013, Electron 1.1 and later does require Visual Studio 2015.

## Getting the Code

```powershell
$ git clone https://github.com/electron/electron.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
the build project files. Notice that we're using `ninja` to build Electron so
there is no Visual Studio project generated.

```powershell
$ cd electron
$ python script\bootstrap.py -v
```

## Building

Build both Release and Debug targets:

```powershell
$ python script\build.py
```

You can also only build the Debug target:

```powershell
$ python script\build.py -c D
```

After building is done, you can find `electron.exe` under `out\D` (debug
target) or under `out\R` (release target).

## 32bit Build

To build for the 32bit target, you need to pass `--target_arch=ia32` when
running the bootstrap script:

```powershell
$ python script\bootstrap.py -v --target_arch=ia32
```

The other building steps are exactly the same.

## Visual Studio project

To generate a Visual Studio project, you can pass the `--msvs` parameter:

```powershell
$ python script\bootstrap.py --msvs
```

## Tests

Test your changes conform to the project coding style using:

```powershell
$ python script\cpplint.py
```

Test functionality using:

```powershell
$ python script\test.py
```

Tests that include native modules (e.g. `runas`) can't be executed with the
debug build (see [#2558](https://github.com/electron/electron/issues/2558) for
details), but they will work with the release build.

To run the tests with the release build use:

```powershell
$ python script\test.py -R
```

## Troubleshooting

### Command xxxx not found

If you encountered an error like `Command xxxx not found`, you may try to use
the `VS2015 Command Prompt` console to execute the build scripts.

### Fatal internal compiler error: C1001

Make sure you have the latest Visual Studio update installed.

### Assertion failed: ((handle))->activecnt >= 0

If building under Cygwin, you may see `bootstrap.py` failed with following
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

This is caused by a bug when using Cygwin Python and Win32 Node together. The
solution is to use the Win32 Python to execute the bootstrap script (assuming
you have installed Python under `C:\Python27`):

```powershell
$ /cygdrive/c/Python27/python.exe script/bootstrap.py
```

### LNK1181: cannot open input file 'kernel32.lib'

Try reinstalling 32bit Node.js.

### Error: ENOENT, stat 'C:\Users\USERNAME\AppData\Roaming\npm'

Simply making that directory [should fix the problem](http://stackoverflow.com/a/25095327/102704):

```powershell
$ mkdir ~\AppData\Roaming\npm
```

### node-gyp is not recognized as an internal or external command

You may get this error if you are using Git Bash for building, you should use
PowerShell or VS2015 Command Prompt instead.

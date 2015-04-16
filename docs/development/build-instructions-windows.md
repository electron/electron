# Build instructions (Windows)

## Prerequisites

* Windows 7 / Server 2008 R2 or higher
* Visual Studio 2013 - [download VS 2013 Community Edition for
  free](http://www.visualstudio.com/products/visual-studio-community-vs)
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [Node.js](http://nodejs.org/download/)
* [git](http://git-scm.com)

If you don't have a Windows installation at the moment,
[modern.ie](https://www.modern.ie/en-us/virtualization-tools#downloads) has
timebombed versions of Windows that you can use to build Electron.

The building of Electron is done entirely with command-line scripts, so you
can use any editor you like to develop Electron, but it also means you can
not use Visual Studio for the development. Support of building with Visual
Studio will come in the future.

**Note:** Even though Visual Studio is not used for building, it's still
**required** because we need the build toolchains it provided.

## Getting the code

```powershell
git clone https://github.com/atom/electron.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
build project files. Notice that we're using `ninja` to build Electron so
there is no Visual Studio project generated.

```powershell
cd electron
python script\bootstrap.py -v
```

## Building

Build both Release and Debug targets:

```powershell
python script\build.py
```

You can also only build the Debug target:

```powershell
python script\build.py -c D
```

After building is done, you can find `atom.exe` under `out\D`.

## 64bit build

To build for the 64bit target, you need to pass `--target_arch=x64` when running
the bootstrap script:

```powershell
python script\bootstrap.py -v --target_arch=x64
```

The other building steps are exactly the same.

## Tests

```powershell
python script\test.py
```

## Troubleshooting

### Command xxxx not found

If you encountered an error like `Command xxxx not found`, you may try to use
the `VS2012 Command Prompt` console to execute the build scripts.

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

This is caused by a bug when using Cygwin python and Win32 node together. The
solution is to use the Win32 python to execute the bootstrap script (supposing
you have installed python under `C:\Python27`):

```bash
/cygdrive/c/Python27/python.exe script/bootstrap.py
```

### LNK1181: cannot open input file 'kernel32.lib'

Try reinstalling 32bit node.js.

### Error: ENOENT, stat 'C:\Users\USERNAME\AppData\Roaming\npm'

Simply making that directory [should fix the problem](http://stackoverflow.com/a/25095327/102704):

```powershell
mkdir ~\AppData\Roaming\npm
```

### node-gyp is not recognized as an internal or external command

You may get this error if you are using Git Bash for building, you should use
PowerShell or VS2012 Command Prompt instead.

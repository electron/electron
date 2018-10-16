# Build Instructions (Windows)

Follow the guidelines below for building Electron on Windows.

## Prerequisites

* Windows 10 / Server 2012 R2 or higher
* Visual Studio 2017 15.7.2 or higher - [download VS 2017 Community Edition for
  free](https://www.visualstudio.com/vs/)
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [Node.js](https://nodejs.org/download/)
* [Git](http://git-scm.com)
* [Debugging Tools for Windows](https://msdn.microsoft.com/en-us/library/windows/hardware/ff551063.aspx)
  if you plan on creating a full distribution since `symstore.exe` is used for
  creating a symbol store from `.pdb` files.

If you don't currently have a Windows installation,
[dev.microsoftedge.com](https://developer.microsoft.com/en-us/microsoft-edge/tools/vms/)
has timebombed versions of Windows that you can use to build Electron.

Building Electron is done entirely with command-line scripts and cannot be done
with Visual Studio. You can develop Electron with any editor but support for
building with Visual Studio will come in the future.

**Note:** Even though Visual Studio is not used for building, it's still
**required** because we need the build toolchains it provides.

## Building

See [Build Instructions: GN](build-instructions-gn.md)

## 32bit Build

To build for the 32bit target, you need to pass `target_cpu = "x86"` as a GN
arg. You can build the 32bit target alongside the 64bit target by using a
different output directory for GN, e.g. `out/Release-x86`, with different
arguments.

```powershell
$ gn gen out/Release-x86 --args="import(\"//electron/build/args/release.gn\") target_cpu=\"x86\""
```

The other building steps are exactly the same.

## Visual Studio project

To generate a Visual Studio project, you can pass the `--ide=vs2017` parameter
to `gn gen`:

```powershell
$ gn gen out/Debug --ide=vs2017
```

## Troubleshooting

### Command xxxx not found

If you encountered an error like `Command xxxx not found`, you may try to use
the `VS2015 Command Prompt` console to execute the build scripts.

### Fatal internal compiler error: C1001

Make sure you have the latest Visual Studio update installed.

### LNK1181: cannot open input file 'kernel32.lib'

Try reinstalling 32bit Node.js.

### Error: ENOENT, stat 'C:\Users\USERNAME\AppData\Roaming\npm'

Creating that directory [should fix the problem](https://stackoverflow.com/a/25095327/102704):

```powershell
$ mkdir ~\AppData\Roaming\npm
```

### node-gyp is not recognized as an internal or external command

You may get this error if you are using Git Bash for building, you should use
PowerShell or VS2015 Command Prompt instead.

### cannot create directory at '...': Filename too long

node.js has some [extremely long pathnames](https://github.com/electron/node/tree/electron/deps/npm/node_modules/libnpx/node_modules/yargs/node_modules/read-pkg-up/node_modules/read-pkg/node_modules/load-json-file/node_modules/parse-json/node_modules/error-ex/node_modules/is-arrayish), and by default git on windows doesn't handle long pathnames correctly (even though windows supports them). This should fix it:

```sh
$ git config --system core.longpaths true
```

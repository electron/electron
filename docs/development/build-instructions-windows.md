# Build Instructions (Windows)

Follow the guidelines below for building **Electron itself** on Windows, for the purposes of creating custom Electron binaries. For bundling and distributing your app code with the prebuilt Electron binaries, see the [application distribution][application-distribution] guide.

[application-distribution]: ../tutorial/application-distribution.md

## Prerequisites

* Windows 10 or later
* [Visual Studio 2026](https://visualstudio.microsoft.com/vs/community/) (>=18.0.0), with the components listed in
  [the Chromium build documentation](https://chromium.googlesource.com/chromium/src/+/main/docs/windows_build_instructions.md#visual-studio).
  If it is installed outside the default location, set `vs2026_install` to its path.
* [Windows 11 SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/) 10.0.28000.2270 and
  SDK Debugging Tools 10.0.26100.3323 or later (provides `symstore.exe` for full distributions).
  See the [Chromium build documentation](https://chromium.googlesource.com/chromium/src/+/main/docs/windows_build_instructions.md) for installation.
  If the SDK is installed outside the default location, set `WINDOWSSDKDIR`.
* [Node.js](https://nodejs.org/download/) 22.22.1 or later
* [Git](https://git-scm.com)

If you don't currently have a Windows installation,
[developer.microsoft.com](https://developer.microsoft.com/en-us/windows/downloads/virtual-machines/)
has timebombed versions of Windows that you can use to build Electron.

Electron is built with GN and command-line tools rather than the Visual Studio
build system. You can develop Electron with any editor.

> [!NOTE]
> Even though Visual Studio is not used for building, it's still
> **required** because we need the build toolchains it provides.

## Exclude source tree from Windows Security

Windows Security doesn't like one of the files in the Chromium source code
(see https://crbug.com/441184), so it will constantly delete it, causing `gclient sync` issues.
You can exclude the source tree from being monitored by Windows Security by
[following these instructions](https://support.microsoft.com/en-us/windows/add-an-exclusion-to-windows-security-811816c0-4dfd-af4a-47e4-c301afe13b26).

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

To generate a Visual Studio project in the latest format supported by GN, you can
pass the `--ide=vs` parameter to `gn gen`:

```powershell
$ gn gen out\Testing --ide=vs
```

## Troubleshooting

### Command xxxx not found

If you encountered an error like `Command xxxx not found`, you may try to use
the `Developer Command Prompt for VS 2026` console to execute the build scripts.

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
PowerShell or Developer Command Prompt for VS 2026 instead.

### cannot create directory at '...': Filename too long

node.js has some [extremely long pathnames](https://github.com/electron/node/tree/electron/deps/npm/node_modules/libnpx/node_modules/yargs/node_modules/read-pkg-up/node_modules/read-pkg/node_modules/load-json-file/node_modules/parse-json/node_modules/error-ex/node_modules/is-arrayish), and by default git on windows doesn't handle long pathnames correctly (even though windows supports them). This should fix it:

```sh
$ git config --system core.longpaths true
```

### error: use of undeclared identifier 'DefaultDelegateCheckMode'

This can happen during build, when Debugging Tools for Windows has been installed with Windows Driver Kit. Uninstall Windows Driver Kit and install Debugging Tools with steps described above.

### Build Scripts Hang Until Keypress

This bug is a "feature" of Windows' command prompt. It happens when clicking inside the prompt window with
`QuickEdit` enabled and is intended to allow selecting and copying output text easily.
Since each accidental click will pause the build process, you might want to disable this
feature in the command prompt properties.

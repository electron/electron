# Build Instructions (macOS)

Follow the guidelines below for building **Electron itself** on macOS, for the purposes of creating custom Electron binaries. For bundling and distributing your app code with the prebuilt Electron binaries, see the [application distribution][application-distribution] guide.

[application-distribution]: ../tutorial/application-distribution.md

## Prerequisites

* macOS >= 11.6.0
* [Xcode](https://developer.apple.com/technologies/tools/). The exact version
  needed depends on what branch you are building, but the latest version of
  Xcode is generally a good bet for building `main`.
* [node.js](https://nodejs.org) (external)
* Python >= 3.7

### Arm64-specific prerequisites

* Rosetta 2
  * We recommend installing Rosetta if using dependencies that need to cross-compile on x64 and arm64 machines. Rosetta can be installed by using the softwareupdate command line tool.
  * `$ softwareupdate --install-rosetta`

## Building Electron

See [Build Instructions: GN](build-instructions-gn.md).

## Troubleshooting

### Xcode "incompatible architecture" errors (MacOS arm64-specific)

If both Xcode and Xcode command line tools are installed (`$ xcode -select --install`, or directly download the correct version [here](https://developer.apple.com/download/all/?q=command%20line%20tools)), but the stack trace says otherwise like so:

```sh
xcrun: error: unable to load libxcrun
(dlopen(/Users/<user>/.electron_build_tools/third_party/Xcode/Xcode.app/Contents/Developer/usr/lib/libxcrun.dylib (http://xcode.app/Contents/Developer/usr/lib/libxcrun.dylib), 0x0005):
 tried: '/Users/<user>/.electron_build_tools/third_party/Xcode/Xcode.app/Contents/Developer/usr/lib/libxcrun.dylib (http://xcode.app/Contents/Developer/usr/lib/libxcrun.dylib)'
 (mach-o file, but is an incompatible architecture (have (x86_64), need (arm64e))), '/Users/<user>/.electron_build_tools/third_party/Xcode/Xcode-11.1.0.app/Contents/Developer/usr/lib/libxcrun.dylib (http://xcode-11.1.0.app/Contents/Developer/usr/lib/libxcrun.dylib)' (mach-o file, but is an incompatible architecture (have (x86_64), need (arm64e)))).`
```

If you are on arm64 architecture, the build script may be pointing to the wrong Xcode version (11.x.y doesn't support arm64). Navigate to `/Users/<user>/.electron_build_tools/third_party/Xcode/` and rename `Xcode-13.3.0.app` to `Xcode.app` to ensure the right Xcode version is used.

### Certificates fail to verify

installing [`certifi`](https://pypi.org/project/certifi/) will fix the following error:

```sh
________ running 'python3 src/tools/clang/scripts/update.py' in '/Users/<user>/electron'
Downloading https://commondatastorage.googleapis.com/chromium-browser-clang/Mac_arm64/clang-llvmorg-15-init-15652-g89a99ec9-1.tgz
<urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:997)>
Retrying in 5 s ...
Downloading https://commondatastorage.googleapis.com/chromium-browser-clang/Mac_arm64/clang-llvmorg-15-init-15652-g89a99ec9-1.tgz
<urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:997)>
Retrying in 10 s ...
Downloading https://commondatastorage.googleapis.com/chromium-browser-clang/Mac_arm64/clang-llvmorg-15-init-15652-g89a99ec9-1.tgz
<urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:997)>
Retrying in 20 s ...
```

This issue has to do with Python 3.6 using its [own](https://github.com/python/cpython/blob/560ea272b01acaa6c531cc7d94331b2ef0854be6/Mac/BuildScript/resources/ReadMe.rtf#L35) copy of OpenSSL in lieu of the deprecated Apple-supplied OpenSSL libraries. `certifi` adds a curated bundle of default root certificates. This issue is documented in the Electron repo [here](https://github.com/electron/build-tools/issues/55). Further information about this issue can be found [here](https://stackoverflow.com/questions/27835619/urllib-and-ssl-certificate-verify-failed-error) and [here](https://stackoverflow.com/questions/40684543/how-to-make-python-use-ca-certificates-from-mac-os-truststore).

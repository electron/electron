# Build Instructions (macOS)

Follow the guidelines below for building **Electron itself** on macOS, for the purposes of creating custom Electron binaries. For bundling and distributing your app code with the prebuilt Electron binaries, see the [application distribution][application-distribution] guide.

[application-distribution]: ../tutorial/application-distribution.md

## Prerequisites

* macOS >= 13 to run Electron; the build host must support the selected Xcode version.
* [Xcode](https://developer.apple.com/technologies/tools/) with the macOS SDK
  recommended by Chromium for the branch you are building.
  The SDK version for official builds is specified in `build/config/mac/mac_sdk.gni`
  in the Chromium checkout.
  * See [the Chromium build documentation](https://chromium.googlesource.com/chromium/src/+/main/docs/mac_build_instructions.md)
  for SDK requirements.
* Python 3.10 through 3.15 for Node.js development and testing. Chromium downloads a
  separate hermetic Python for GN build scripts.
* [Node.js](https://nodejs.org/download/) 22.22.1 or later.

### Arm64-specific prerequisites

* Rosetta 2 is required when running x64-only build tools on Apple silicon,
  including for x64 release builds.
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

On Apple silicon, this error can mean that the build is using an Xcode toolchain
with x64-only binaries. Make sure the configured Xcode installation supports
your host architecture and includes the required macOS SDK.

### Certificates fail to verify

Installing [`certifi`](https://pypi.org/project/certifi/) will fix the following error:

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

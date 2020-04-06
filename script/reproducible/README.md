Reproducible builds are a set of software development practices that
create an independently-verifiable path from source to binary code.
See [https://reproducible-builds.org/](https://reproducible-builds.org/)
for more information.

Electron release determinism may be verified by calling one of the two verify scripts
in this directory.

In particular on macOS and Linux, an user may invoke

```bash
$ ./verify.sh [-b $buildir] [-n] [CHECK_VERSION]
```

Where:

* `-b` build directory in which electron will be build
* `-n` don't check for minimum required space, in this case 200GB
* The first argument `CHECK_VERSION` represents a given tag of which the user is willing to verify determinism (eg. `v10.0.0`)

While on Windows 
```bash
$ verify.bat [CHECK_VERSION] [-buildir $buildir]
```

Where:

* `-buildir` build directory in which electron will be build
* The first argument `CHECK_VERSION` represents a given tag of which the user is willing to verify determinism (eg. `v10.0.0`)

## Notes

It is important to note that while Linux releases is, at the time of writing,
generating path independant symbols, it is not the case on Windows and macOS where the output
directory will be important to be set respectively to `C:\projects\src`
and `/Users/distiller/project/src` as reported in the current `toolchain_profile.json`.

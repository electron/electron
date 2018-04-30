# Electron Support

## Supported Versions

The Electron maintainers support the latest three release branches.
For example, if the latest release is 2.0.x, then the 2-0-x series
is supported, as are the two previous release series 1-7-x and 1-8-x.

When a release branch reaches the end of its support cycle,
the series will be deprecated in NPM and a final end-of-support release
will be made. This release will add a console warning to inform that
an unsupported version of Electron is in use.

These steps are to help app developers learn when a branch they're using
becomes unsupported, and to avoid being intrusive to end users.

If an application has exceptional circumstances and needs to stay
on an unsupported series of Electron, developers can silence the
end-of-support warning by omitting the final release from the app's
`package.json` `devDependencies`. For example, since the 1-6-x series
ended with an end-of-support 1.6.18 release, developers could choose
to stay in the 1-6-x series without warnings with `devDependency  of
`"electron": 1.6.0 - 1.6.17`.


## Supported Platforms

Following platforms are supported by Electron:

### macOS

Only 64bit binaries are provided for macOS, and the minimum macOS version
supported is macOS 10.9.

### Windows

Windows 7 and later are supported, older operating systems are not supported
(and do not work).

Both `ia32` (`x86`) and `x64` (`amd64`) binaries are provided for Windows.
Please note, the `ARM` version of Windows is not supported for now.

### Linux

The prebuilt `ia32` (`i686`) and `x64` (`amd64`) binaries of Electron are built on
Ubuntu 12.04, the `armv7l` binary is built against ARM v7 with hard-float ABI and
NEON for Debian Wheezy.

[Until the release of Electron 2.0][arm-breaking-change], Electron will also
continue to release the `armv7l` binary with a simple `arm` suffix. Both binaries 
are identical.

Whether the prebuilt binary can run on a distribution depends on whether the
distribution includes the libraries that Electron is linked to on the building
platform, so only Ubuntu 12.04 is guaranteed to work, but following platforms
are also verified to be able to run the prebuilt binaries of Electron:

* Ubuntu 12.04 and newer
* Fedora 21
* Debian 8

[arm-breaking-change]: https://github.com/electron/electron/blob/master/docs/tutorial/planned-breaking-changes.md#duplicate-arm-assets

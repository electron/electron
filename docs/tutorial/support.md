# Electron Support

## Finding Support

If you have a security concern,
please see the [security document](../../SECURITY.md).

If you're looking for programming help,
for answers to questions,
or to join in discussion with other developers who use Electron,
you can interact with the community in these locations:
- [`electron`](https://discuss.atom.io/c/electron) category on the Atom
forums
- `#atom-shell` channel on Freenode
- [`Electron`](https://atom-slack.herokuapp.com) channel on Atom's Slack
- [`electron-ru`](https://telegram.me/electron_ru) *(Russian)*
- [`electron-br`](https://electron-br.slack.com) *(Brazilian Portuguese)*
- [`electron-kr`](https://electron-kr.github.io/electron-kr) *(Korean)*
- [`electron-jp`](https://electron-jp.slack.com) *(Japanese)*
- [`electron-tr`](https://electron-tr.herokuapp.com) *(Turkish)*
- [`electron-id`](https://electron-id.slack.com) *(Indonesia)*
- [`electron-pl`](https://electronpl.github.io) *(Poland)*

If you'd like to contribute to Electron,
see the [contributing document](../../CONTRIBUTING.md).

If you've found a bug in a [supported version](#supported-versions) of Electron,
please report it with the [issue tracker](../development/issues.md).

[awesome-electron](https://github.com/sindresorhus/awesome-electron)
is a community-maintained list of useful example apps,
tools and resources.

## Supported Versions

The latest three release branches are supported by the Electron team.
For example, if the latest release is 2.0.x, then the 2-0-x series
is supported, as are the two previous release series 1-7-x and 1-8-x.

When a release branch reaches the end of its support cycle, the series
will be deprecated in NPM and a final end-of-support release will be
made. This release will add a warning to inform that an unsupported
version of Electron is in use.

These steps are to help app developers learn when a branch they're
using becomes unsupported, but without being excessively intrusive
to end users.

If an application has exceptional circumstances and needs to stay
on an unsupported series of Electron, developers can silence the
end-of-support warning by omitting the final release from the app's
`package.json` `devDependencies`. For example, since the 1-6-x series
ended with an end-of-support 1.6.18 release, developers could choose
to stay in the 1-6-x series without warnings with `devDependency` of
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
Running Electron apps on Windows for ARM devices is possible by using the
ia32 binary.

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

[arm-breaking-change]: https://github.com/electron/electron/blob/master/docs/api/breaking-changes.md#duplicate-arm-assets

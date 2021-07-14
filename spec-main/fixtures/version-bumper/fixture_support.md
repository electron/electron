# Electron Support

## Finding Support

If you have a security concern,
please see the [security document](https://github.com/electron/electron/tree/master/SECURITY.md).

If you're looking for programming help,
for answers to questions,
or to join in discussion with other developers who use Electron,
you can interact with the community in these locations:

* [`Electron's Discord`](https://discord.com/invite/electron) has channels for:
  * Getting help
  * Ecosystem apps like [Electron Forge](https://github.com/electron-userland/electron-forge) and [Electron Fiddle](https://github.com/electron/fiddle)
  * Sharing ideas with other Electron app developers
  * And more!
* [`electron`](https://discuss.atom.io/c/electron) category on the Atom forums
* `#atom-shell` channel on Freenode
* `#electron` channel on [Atom's Slack](https://discuss.atom.io/t/join-us-on-slack/16638?source_topic_id=25406)
* [`electron-ru`](https://telegram.me/electron_ru) *(Russian)*
* [`electron-br`](https://electron-br.slack.com) *(Brazilian Portuguese)*
* [`electron-kr`](https://electron-kr.github.io/electron-kr) *(Korean)*
* [`electron-jp`](https://electron-jp.slack.com) *(Japanese)*
* [`electron-tr`](https://electron-tr.herokuapp.com) *(Turkish)*
* [`electron-id`](https://electron-id.slack.com) *(Indonesia)*
* [`electron-pl`](https://electronpl.github.io) *(Poland)*

If you'd like to contribute to Electron,
see the [contributing document](https://github.com/electron/electron/blob/master/CONTRIBUTING.md).

If you've found a bug in a [supported version](#supported-versions) of Electron,
please report it with the [issue tracker](../development/issues.md).

[awesome-electron](https://github.com/sindresorhus/awesome-electron)
is a community-maintained list of useful example apps,
tools and resources.

## Supported Versions

The latest three *stable* major versions are supported by the Electron team.
For example, if the latest release is 6.1.x, then the 5.0.x as well
as the 4.2.x series are supported.  We only support the latest minor release
for each stable release series.  This means that in the case of a security fix
6.1.x will receive the fix, but we will not release a new version of 6.0.x.

The latest stable release unilaterally receives all fixes from `master`,
and the version prior to that receives the vast majority of those fixes
as time and bandwidth warrants. The oldest supported release line will receive
only security fixes directly.

All supported release lines will accept external pull requests to backport
fixes previously merged to `master`, though this may be on a case-by-case
basis for some older supported lines. All contested decisions around release
line backports will be resolved by the [Releases Working Group](https://github.com/electron/governance/tree/master/wg-releases) as an agenda item at their weekly meeting the week the backport PR is raised.

When an API is changed or removed in a way that breaks existing functionality, the
previous functionality will be supported for a minimum of two major versions when
possible before being removed. For example, if a function takes three arguments,
and that number is reduced to two in major version 10, the three-argument version would
continue to work until, at minimum, major version 12. Past the minimum two-version
threshold, we will attempt to support backwards compatibility beyond two versions
until the maintainers feel the maintenance burden is too high to continue doing so.

### Currently supported versions

* 4.x.y
* 3.x.y
* 2.x.y
* 1.x.y

### End-of-life

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
supported is macOS 10.11 (El Capitan).

Native support for Apple Silicon (`arm64`) devices was added in Electron 11.0.0.

### Windows

Windows 7 and later are supported, older operating systems are not supported
(and do not work).

Both `ia32` (`x86`) and `x64` (`amd64`) binaries are provided for Windows.
[Native support for Windows on Arm (`arm64`) devices was added in Electron 6.0.8.](windows-arm.md).
Running apps packaged with previous versions is possible using the ia32 binary.

### Linux

The prebuilt binaries of Electron are built on Ubuntu 18.04.

Whether the prebuilt binary can run on a distribution depends on whether the
distribution includes the libraries that Electron is linked to on the building
platform, so only Ubuntu 18.04 is guaranteed to work, but following platforms
are also verified to be able to run the prebuilt binaries of Electron:

* Ubuntu 14.04 and newer
* Fedora 24 and newer
* Debian 8 and newer

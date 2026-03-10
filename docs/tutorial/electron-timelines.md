# Electron Releases

Electron frequently releases major versions alongside every other Chromium release.
This document focuses on the release cadence and version support policy.
For a more in-depth guide on our git branches and how Electron uses semantic versions,
check out our [Electron Versioning](./electron-versioning.md) doc.

## Timeline

[Electron's Release Schedule](https://releases.electronjs.org/schedule) lists a schedule of Electron major releases showing key milestones including alpha, beta, and stable release dates, as well as end-of-life dates and dependency versions.

:::info Official support dates may change

Electron's official support policy is the latest 3 stable releases. Our stable
release and end-of-life dates are determined by Chromium, and may be subject to
change. While we try to keep our planned release and end-of-life dates frequently
updated here, future dates may change if affected by upstream scheduling changes,
and may not always be accurately reflected.

 See [Chromium's public release schedule](https://chromiumdash.appspot.com/schedule) for
 definitive information about Chromium's scheduled release dates.

 :::

**Notes:**

* The `-alpha.1`, `-beta.1`, and `stable` dates are our solid release dates.
* We strive for weekly alpha/beta releases, but we often release more than scheduled.
* All dates are our goals but there may be reasons for adjusting the stable deadline, such as security bugs.

**Historical changes:**

* Since Electron 5, Electron has been publicizing its release dates ([see blog post](https://www.electronjs.org/blog/electron-5-0-timeline)).
* Since Electron 6, Electron major versions have been targeting every other Chromium major version. Each Electron stable should happen on the same day as Chrome stable ([see blog post](https://www.electronjs.org/blog/12-week-cadence)).
* Since Electron 16, Electron has been releasing major versions on an 8-week cadence in accordance to Chrome's change to a 4-week release cadence ([see blog post](https://www.electronjs.org/blog/8-week-cadence)).
* Electron temporarily extended support for Electron 22 until October 10, 2023, to support an extended end-of-life for Windows 7/8/8.1

## Version support policy

The latest three _stable_ major versions are supported by the Electron team.
For example, if the latest release is 6.1.x, then the 5.0.x as well
as the 4.2.x series are supported. We only support the latest minor release
for each stable release series. This means that in the case of a security fix,
6.1.x will receive the fix, but we will not release a new version of 6.0.x.

The latest stable release unilaterally receives all fixes from `main`,
and the version prior to that receives the vast majority of those fixes
as time and bandwidth warrants. The oldest supported release line will receive
only security fixes directly.

### Chromium version support

:::info Chromium release schedule

Chromium's public release schedule is [here](https://chromiumdash.appspot.com/schedule).

:::

Electron targets Chromium even-number versions, releasing every 8 weeks in concert
with Chromium's 4-week release schedule. For example, Electron 26 uses Chromium 116, while Electron 27 uses Chromium 118.

### Node.js version support

Electron upgrades its `main` branch to even-number versions of Node.js when they enter Active LTS. The schedule
is as follows:

<img src="https://raw.githubusercontent.com/nodejs/Release/main/schedule.svg?sanitize=true" alt="Releases">

If Electron has recently updated its `main` branch to a new major version of Node.js, the next stable
branch to be cut will be released with the new version.

Stable release lines of Electron will receive minor and patch bumps of Node.js after they are released.
Patch bumps to Node.js will be released in patch releases of Electron, and minor bumps to Node.js will result in a minor release of Electron.
Security-only release branches will receive security-related changes from Node.js releases, but not the full release.

### Breaking API changes

When an API is changed or removed in a way that breaks existing functionality, the
previous functionality will be supported for a minimum of two major versions when
possible before being removed. For example, if a function takes three arguments,
and that number is reduced to two in major version 10, the three-argument version would
continue to work until, at minimum, major version 12. Past the minimum two-version
threshold, we will attempt to support backwards compatibility beyond two versions
until the maintainers feel the maintenance burden is too high to continue doing so.

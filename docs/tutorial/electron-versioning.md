# Electron Versioning

## Overview of Semantic Versioning

If you've been using Node and npm for a while, you are probably aware of [Semantic Versioning], or SemVer for short. It's a convention for specifying version numbers for software that helps communicate intentions to the users of your software.

Semantic versions are always made up of three numbers:

```
major.minor.patch
```

Semantic version numbers are bumped (incremented) using the following rules:

* **Major** is for changes that break backwards compatibility.
* **Minor** is for new features that don't break backwards compatibility.
* **Patch** is for bug fixes and other minor changes.

A simple mnemonic for remembering this scheme is as follows:

```
breaking.feature.fix
```

## Before Version 2

Before version 2 of Electron we didn't follow SemVer, instead the following was used:

- **Major**: Breaking changes to Electron's API
- **Minor**: Major Chrome, minor node or "significant" Electron changes
- **Patch**: New features and bug fixes

This system had a number of drawbacks, such as:

- New bugs could be introduced into a new patch version because patch versions added features
- It didn't follow SemVer so it could confuse consumers
- It wasn't clear what the differences between stable and beta builds were
- The lack of a formalized stabilization process and release schedule lead to sporadic releases and betas that could last several months 

## Version 2 and Beyond

From version 2, Electron will attempt to adhere to SemVer and follow a release schedule and stabilization process similar to that of Chromium's that addresses some of the issues some Electron apps were experiencing.

*Some of the numbers below may change after a few versions.*

### Version Change Rules

In order to reduce ambiguity in what changes between releases, Electron is going to attempt to align to SemVer as much as possible. Here are the general rules that will be followed:

| Type of change | Version increase
|---|---
| Chromium version update | Major
| Node *major* version update | Major
| Node *minor* version update | Minor
| Electron breaking API change | Major
| Electron non-breaking API change | Minor
| Electron bug fix | Patch
| Anything sufficiently "risky"\* | Major

\* The idea here is to attempt to mitigate regressions in minor and patch versions so applications can update as easily as possible. An example of a risky change is changing the way that builds are linked.

Because SemVer is followed, it's recommended that you lock your electron dependency to the minor version using the `~` annotation. This will allow new patch versions (bug fixes) to be updated automatically.

### The Release Schedule

![](../images/tutorial-release-schedule.svg)

Here are some important points to call out:

- A new release is performed approximately weekly
- Minor versions are branched off of master for stabilization
- The stabilization period is approximately weekly
- Important bug fixes are cherry picked to stabilization branches after landing in master
- Features are not cherry picked, a minor version should only get *more stable* with its patch versions
- There is little difference in the release schedule between a major and minor release, other than the risk/effort it may take for third parties to adopt
- Chromium updates will be performed as fast as the team can manage, in an ideal world this would happen every 6 weeks to align with [Chromium's release schedule][Chromium release]
- Excluding exceptional circumstances, only the previous stable build will get backported bug fixes.

### The Beta Process

Electron relies on its consumers getting involved in stabilization. The short target stabilization period and rapid release cadence was designed to shipping security and bug fixes out fast and to encourage the automation of testing.

You can install the beta by specifying the `beta` dist tag when installing via npm:

```
npm install electron@beta
```

[Semantic Versioning]: http://semver.org
[pre-release identifier]: http://semver.org/#spec-item-9
[npm dist tag]: https://docs.npmjs.com/cli/dist-tag
[normal version]: http://semver.org/#spec-item-2
[Chromium release]: https://www.chromium.org/developers/calendar
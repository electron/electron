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

From version 2.0.0, Electron will attempt to adhere to SemVer and follow a 
release schedule and stabilization process similar to that of Chromium.

### Version Change Rules

Here are the general rules that apply when releasing new versions:

| Type of change | Version increase
|---|---
| Chromium version update | Major
| Node *major* version update | Major
| Electron breaking API change | Major
| Any other changes deemed "risky" | Major
| Node *minor* version update | Minor
| Electron non-breaking API change | Minor
| Electron bug fix | Patch

When you install an npm module with the `--save` or `--save-dev` flags, it
will be prefixed with a caret `^` in package.json:

```json
{
  "devDependencies": {
    "electron": "^2.0.0"
  }
}
```

The [caret semver range](https://docs.npmjs.com/misc/semver#caret-ranges-123-025-004) 
allows minor- and patch-level changes to be installed, i.e. non-breaking 
features and bug fixes.

Alternatively, a more conservative approach is to use the 
[tilde semver range](https://docs.npmjs.com/misc/semver#tilde-ranges-123-12-1)
`~`, which will only allow patch-level upgrades, i.e. bug fixes.


### The Release Schedule

**Note: The schedule outlined here is _aspirational_. We are not yet cutting
releases at a weekly cadence, but we hope to get there eventually.**

<img style="width:100%;margin:20px 0;" src="../images/tutorial-release-schedule.svg">

Here are some important points to call out:

- A new release is performed approximately weekly.
- Minor versions are branched off of master for stabilization.
- The stabilization period is approximately weekly.
- Important bug fixes are cherry-picked to stabilization branches after landing 
  in master.
- Features are not cherry picked; a minor version should only get *more stable* 
  with its patch versions.
- There is little difference in the release schedule between a major and minor 
  release, other than the risk/effort it may take for third parties to adopt
- Chromium updates will be performed as fast as the team can manage. In an ideal 
  world this would happen every 6 weeks to align with 
  [Chromium's release schedule][Chromium release].
- Excluding exceptional circumstances, only the previous stable build will 
  get backported bug fixes.

### The Beta Process

Electron relies on its consumers getting involved in stabilization. The short 
target stabilization period and rapid release cadence was designed for shipping 
security and bug fixes out fast and to encourage the automation of testing.

You can install the beta by specifying the `beta` dist tag when installing via 
npm:

```
npm install electron@beta
```

[Semantic Versioning]: http://semver.org
[pre-release identifier]: http://semver.org/#spec-item-9
[npm dist tag]: https://docs.npmjs.com/cli/dist-tag
[normal version]: http://semver.org/#spec-item-2
[Chromium release]: https://www.chromium.org/developers/calendar
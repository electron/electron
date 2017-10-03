# Electron Versioning

If you've been using Node and npm for a while, you are probably aware of [Semantic Versioning], or SemVer for short. It's a convention for specifying version numbers for software that helps communicate intentions to the users of your software.

## Overview of Semantic Versioning

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

## Electron Versioning

Due to its dependency on Node and Chromium, it is not possible for the Electron
project to adhere to a SemVer policy. **You should therefore always
reference a specific version of Electron.**

Electron version numbers are bumped using the following rules:

* **Major** is for breaking changes in Electron's API. If you upgrade from `0.37.0`
  to `1.0.0`, you will have to make changes to your app.
* **Minor** is for major Chrome and minor Node upgrades, or significant Electron
  changes. If you upgrade from `1.5.0` to `1.6.0`, your app is supposed to
  still work, but you might have to work around small changes.
* **Patch** is for new features and bug fixes. If you upgrade from `1.6.2` to
  `1.6.3`, your app will continue to work as-is.

We recommend that you set a fixed version when installing Electron from npm:

```sh
npm install electron --save-exact --save-dev
```

The `--save-exact` flag will add `electron` to your `package.json` file without
using a `^` or `~`, e.g. `1.6.2` instead of `^1.6.2`. This practice ensures that
all upgrades of Electron are a manual operation made by you, the developer.

Alternatively, you can use the `~` prefix in your SemVer range, like `~1.6.2`.
This will lock your major and minor version, but allow new patch versions to
be installed.

[Semantic Versioning]: http://semver.org

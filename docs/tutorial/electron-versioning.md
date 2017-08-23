# Electron Versioning

If you've been using Node and npm for a while, you are probably aware of [Semantic Versioning], or SemVer for short. It's a convention for specifying version numbers for software that helps communicate intentions to the users of your software.

Due to its dependency on Node and Chromium, it is not possible for the Electron
project to adhere to a strict [Semantic Versioning] policy. **You should 
therefore always reference a specific version of Electron** in your
`package.json` file.

Electron version numbers are bumped using the following rules:

* **Major** is for breaking changes in Electron's API. If you upgrade from `0.37.0`
  to `1.0.0`, you will have to make changes to your app.
* **Minor** is for major Chrome and minor Node upgrades, or significant Electron
  changes. If you upgrade from `1.5.0` to `1.6.0`, your app should
  still work, but you might have to work around small changes.
* **Patch** is for new features and bug fixes. If you upgrade from `1.6.2` to
  `1.6.3`, your app should continue to work as-is.

We recommend that you set a fixed version when installing Electron from npm:

```sh
npm install electron --save-exact --save-dev
```

The `--save-exact` flag will add `electron` to your `package.json` file _without
a range identifier_ like `^` or `~`, e.g. `1.6.2` instead of `^1.6.2`. This 
practice ensures that all upgrades of Electron are a manual operation made by 
you, the developer.

Alternatively, you can use the `~` prefix in your SemVer range, like `~1.6.2`.
This will lock your major and minor version, but allow new patch versions to
be installed.

## Prereleases

Starting at version 1.8, unstable releases of Electron have a suffix called a
[pre-release identifier] appended to their version number, 
e.g. `1.8.0-beta.0`. A version may have many prereleases before it is 
considered stable, e.g. `1.8.0-beta.0`, `1.8.0-beta.1`, and eventually `1.8.0`.

When major, minor, and patch are equal, a pre-release version has lower 
precedence than a [normal version], e.g. `1.8.0-beta.0 < 1.8.0`. This is 
convenient because it allows you to use a range like `^1.8.0` and know 
that it will never match an unstable pre-release version.

The `latest` and `next` [npm dist tags] are also used:

- `npm install electron@latest` will install the latest _stable_ version.
- `npm install electron@next` will install the very latest _unstable_ version.

## Stable Releases

In general, a version is considered stable after its most recent 
[prerelease](#prereleases) has been out for two weeks and any significant bugs 
reported against it have been fixed. Note that versions are not promoted on a 
set schedule, and timing can fluctuate per release.

We recommend using the following command to ensure you're using a stable 
version of Electron:

```sh
npm install electron --save-exact --save-dev
```

If you have an existing Electron app and want to update it to use the latest 
stable version of `electron`, use the `@latest` identifier:

```sh
npm install electron@latest --save-exact --save-dev
```

[Semantic Versioning]: http://semver.org
[pre-release identifier]: http://semver.org/#spec-item-9
[npm dist tags]: https://docs.npmjs.com/cli/dist-tag
[normal version]: http://semver.org/#spec-item-2
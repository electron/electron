# Electron Versioning

> A detailed look at our versioning policy and implementation.

As of version 2.0.0, Electron follows [semver](#semver). The following command will install the most recent stable build of Electron:

```sh
npm install --save-dev electron
```

To update an existing project to use the latest stable version:

```sh
npm install --save-dev electron@latest
```

## Version 1.x

Electron versions *< 2.0* did not conform to the [semver](http://semver.org) spec: major versions corresponded to end-user API changes, minor versions corresponded to Chromium major releases, and patch versions corresponded to new features and bug fixes. While convenient for developers merging features, it creates problems for developers of client-facing applications. The QA testing cycles of major apps like Slack, Stride, Teams, Skype, VS Code, Atom, and Desktop can be lengthy and stability is a highly desired outcome. There is a high risk in adopting new features while trying to absorb bug fixes.

Here is an example of the 1.x strategy:

![](../images/versioning-sketch-0.png)

An app developed with `1.8.1` cannot take the `1.8.3` bug fix without either absorbing the `1.8.2` feature, or by backporting the fix and maintaining a new release line.

## Version 2.0 and Beyond

There are several major changes from our 1.x strategy outlined below. Each change is intended to satisfy the needs and priorities of developers/maintainers and app developers.

1. Strict use of semver
2. Introduction of semver-compliant `-beta` tags
3. Introduction of [conventional commit messages](https://conventionalcommits.org/)
4. Well-defined stabilization branches
5. The `master` branch is versionless; only stabilization branches contain version information

We will cover in detail how git branching works, how npm tagging works, what developers should expect to see, and how one can backport changes.

# semver

From 2.0 onward, Electron will follow semver.

Below is a table explicitly mapping types of changes to their corresponding category of semver (e.g. Major, Minor, Patch).

| Major Version Increments        | Minor Version Increments           | Patch Version Increments      |
| ------------------------------- | ---------------------------------- | ----------------------------- |
| Electron breaking API changes   | Electron non-breaking API changes  | Electron bug fixes            |
| Node.js major version updates   | Node.js minor version updates      | Node.js patch version updates |
| Chromium version updates        |                                    | fix-related chromium patches  |


Note that most chromium updates will be considered breaking. Fixes that can be backported will likely be cherry-picked as patches.

# Stabilization Branches

Stabilization branches are branches that run parallel to master, taking in only cherry-picked commits that are related to security or stability. These branches are never merged back to master.

![](../images/versioning-sketch-1.png)

Stabilization branches are always either **major** or **minor** version lines, and named against the following template `$MAJOR-$MINOR-x` e.g. `2-0-x`.

We allow for multiple stabilization branches to exist simultaneously, and intend to support at least two in parallel at all times, backporting security fixes as necessary.
![](../images/versioning-sketch-2.png)

Older lines will not be supported by GitHub, but other groups can take ownership and backport stability and security fixes on their own. We discourage this, but recognize that it makes life easier for many app developers.

# Beta Releases and Bug Fixes

Developers want to know which releases are _safe_ to use. Even seemingly innocent features can introduce regressions in complex applications. At the same time, locking to a fixed version is dangerous because you’re ignoring security patches and bug fixes that may have come out since your version. Our goal is to allow the following standard semver ranges in `package.json` :

* Use `~2.0.0` to admit only stability or security related fixes to your `2.0.0` release.
* Use `^2.0.0` to admit non-breaking _reasonably stable_ feature work as well as security and bug fixes.

What’s important about the second point is that apps using `^` should still be able to expect a reasonable level of stability. To accomplish this, semver allows for a _pre-release identifier_ to indicate a particular version is not yet _safe_ or _stable_.

Whatever you choose, you will periodically have to bump the version in your `package.json` as breaking changes are a fact of Chromium life.

The process is as follows:

1. All new major and minor releases lines begin with a beta series indicated by semver prerelease tags of `beta.N`, e.g. `2.0.0-beta.1`. After the first beta, subsequent beta releases must meet all of the following conditions:
    1. The change is backwards API-compatible (deprecations are allowed)
    2. The risk to meeting our stability timeline must be low.
2. If allowed changes need to be made once a release is beta, they are applied and the prerelease tag is incremented, e.g. `2.0.0-beta.2`.
3. If a particular beta release is _generally regarded_ as stable, it will be re-released as a stable build, changing only the version information. e.g. `2.0.0`. After the first stable, all changes must be backwards-compatible bug or security fixes.
4. If future bug fixes or security patches need to be made once a release is stable, they are applied and the _patch_ version is incremented
e.g. `2.0.1`.

Specifically, the above means:

1. Admitting non-breaking-API changes early in the beta cycle is okay, even if those changes have the potential to cause moderate side-affects
2. Admitting feature-flagged changes, that do not otherwise alter existing code paths, at most points in the beta cycle is okay. Users can explicitly enable those flags in their apps.
3. Admitting features of any sort very late in the beta cycle is 👎 without a very good reason.

For each major and minor bump, you should expect to see something like the following:

```text
2.0.0-beta.1
2.0.0-beta.2
2.0.0-beta.3
2.0.0
2.0.1
2.0.2
```

An example lifecycle in pictures:

* A new release branch is created that includes the latest set of features. It is published as `2.0.0-beta.1`.
![](../images/versioning-sketch-3.png)
* A bug fix comes into master that can be backported to the release branch. The patch is applied, and a new beta is published as `2.0.0-beta.2`.
![](../images/versioning-sketch-4.png)
* The beta is considered _generally stable_ and it is published again as a non-beta under `2.0.0`.
![](../images/versioning-sketch-5.png)
* Later, a zero-day exploit is revealed and a fix is applied to master. We backport the fix to the `2-0-x` line and release `2.0.1`.
![](../images/versioning-sketch-6.png)

A few examples of how various semver ranges will pick up new releases:

![](../images/versioning-sketch-7.png)

# Missing Features: Alphas, and Nightly
Our strategy has a few tradeoffs, which for now we feel are appropriate. Most importantly that new features in master may take a while before reaching a stable release line. If you want to try a new feature immediately, you will have to build Electron yourself.

As a future consideration, we may introduce one or both of the following:

* nightly builds off of master; these would allow folks to test new features quickly and give feedback
* alpha releases that have looser stability constraints to betas; for example it would be allowable to admit new features while a stability channel is in _alpha_

# Feature Flags
Feature flags are a common practice in Chromium, and are well-established in the web-development ecosystem. In the context of Electron, a feature flag or **soft branch** must have the following properties:

* it is enabled/disabled either at runtime, or build-time; we do not support the concept of a request-scoped feature flag
* it completely segments new and old code paths; refactoring old code to support a new feature _violates_ the feature-flag contract
* feature flags are eventually removed after the feature is released

# Semantic Commits

We seek to increase clarity at all levels of the update and releases process. Starting with `2.0.0` we will require pull requests adhere to the [Conventional Commits](https://conventionalcommits.org/) spec, which can be summarized as follows:

* Commits that would result in a semver **major** bump must start with `BREAKING CHANGE:`.
* Commits that would result in a semver **minor** bump must start with `feat:`.
* Commits that would result in a semver **patch** bump must start with `fix:`.

* We allow squashing of commits, provided that the squashed message adheres the the above message format.
* It is acceptable for some commits in a pull request to not include a semantic prefix, as long as a later commit in the same pull request contains a meaningful encompassing semantic message.

# Versionless `master`

- The `master` branch will always contain `0.0.0-dev` in its `package.json`
- Release branches are never merged back to master
- Release branches _do_ contain the correct version in their `package.json`

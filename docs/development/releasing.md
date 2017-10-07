# Releasing

This document describes the process for releasing a new version of Electron.

## Find out what version change is needed
Is this a major, minor, patch, or beta version change? Read the [Version Change Rules](docs/tutorial/electron-versioning.md#version-change-rules) to find out.

## Create a temporary branch

- **If releasing beta,** create a new branch from `master`.
- **If releasing a stable version,** create a new branch from the beta branch you're stablizing.

Name the new branch `release` or anything you like.

```sh
git checkout master
git pull
git checkout -b release
```

This branch is created as a precaution to prevent any merged PRs from sneaking into a release between the time the temporary release branch is created and the CI builds are complete.

## Check for extant drafts

The upload script [looks for an existing draft release](https://github.com/electron/electron/blob/7961a97d7ddbed657c6c867cc8426e02c236c077/script/upload.py#L173-L181). To prevent your new release
from clobbering an existing draft, check [the releases page] and
make sure there are no drafts.

## Bump the version

Run the `bump-version` script with arguments according to your need:
- `--bump=[major|minor|patch|beta]` to increment one of the version numbers, or
- `--stable` to indicate this is a stable version, or
- `--version={version}` to set version number directly.

**Note**: you can use both `--bump` and `--stable` simultaneously.

There is also a `dry-run` flag you can use to make sure the version number generated is correct before committing.

```sh
npm run bump-version -- --bump=patch --stable
git push origin HEAD
```

This will bump the version number in several files. See [this bump commit] for an example.

## Wait for builds :hourglass_flowing_sand:

The presence of the word [`Bump`](https://github.com/electron/electron/blob/7961a97d7ddbed657c6c867cc8426e02c236c077/script/cibuild-linux#L3-L6) in the commit message created by the `bump-version` script
will [trigger the release process](https://github.com/electron/electron/blob/7961a97d7ddbed657c6c867cc8426e02c236c077/script/cibuild#L82-L96).

To monitor the build progress, see the following pages:

- [208.52.191.140:8080/view/All/builds](http://208.52.191.140:8080/view/All/builds) for Mac
- [circleci.com/gh/electron](https://circleci.com/gh/electron) for Linux
- [windows-ci.electronjs.org/project/AppVeyor/electron](https://windows-ci.electronjs.org/project/AppVeyor/electron) for Windows

## Compile release notes

Writing release notes is a good way to keep yourself busy while the builds are running.
For prior art, see existing releases on [the releases page].

Tips:
- Each listed item should reference a PR on electron/electron, not an issue, nor a PR from another repo like libcc.
- No need to use link markup when referencing PRs. Strings like `#123` will automatically be converted to links on github.com.
- To see the version of Chromium, V8, and Node in every version of Electron, visit [atom.io/download/electron/index.json](https://atom.io/download/electron/index.json).

### Patch releases

For a `patch` release, use the following format:

```
## Bug Fixes

* Fixed a cross-platform thing. #123

### Linux

* Fixed a Linux thing. #123

### macOS

* Fixed a macOS thing. #123

### Windows

* Fixed a Windows thing. #1234
```

### Minor releases

For a `minor` release, e.g. `1.8.0`, use this format:

```
## Upgrades

- Upgraded from Node `oldVersion` to `newVersion`. #123

## API Changes

* Changed a thing. #123

### Linux

* Changed a Linux thing. #123

### macOS

* Changed a macOS thing. #123

### Windows

* Changed a Windows thing. #123
```

### Major releases
```
## Upgrades

- Upgraded from Chromium `oldVersion` to `newVersion`. #123
- Upgraded from Node `oldVersion` to `newVersion`. #123

## Breaking API changes

* Changed a thing. #123

### Linux

* Changed a Linux thing. #123

### macOS

* Changed a macOS thing. #123

### Windows

* Changed a Windows thing. #123

## Other Changes

- Some other change. #123
```

### Beta releases
Use the same formats as the ones suggested above, but add the following note at the beginning of the changelog:
```
**Note:** This is a beta release and most likely will have have some instability and/or regressions.

Please file new issues for any bugs you find in it.

This release is published to [npm](https://www.npmjs.com/package/electron) under the `beta` tag and can be installed via `npm install electron@beta`.
```


## Edit the release draft

1. Visit [the releases page] and you'll see a new draft release with placeholder release notes.
1. Edit the release and add release notes.
1. Uncheck the `prerelease` checkbox if you're publishing a stable release; leave it checked for beta releases.
1. Click 'Save draft'. **Do not click 'Publish release'!**
1. Wait for all builds to pass before proceeding.

## Merge temporary branch

Merge the temporary branch back into master, without creating a merge commit:

```sh
git checkout master
git merge release --no-commit
git push origin master
```

If this fails, rebase with master and rebuild:

```sh
git pull
git checkout release
git rebase master
git push origin HEAD
```

## Run local debug build

Run local debug build to verify that you are actually building the version you want. Sometimes you thought you were doing a release for a new version, but you're actually not.

```sh
npm run build
npm start
```

Verify the window is displaying the current updated version.

## Set environment variables

You'll need to set the following environment variables to publish a release. Ask another team member for these credentials.

- `ELECTRON_S3_BUCKET`
- `ELECTRON_S3_ACCESS_KEY`
- `ELECTRON_S3_SECRET_KEY`
- `ELECTRON_GITHUB_TOKEN` - A personal access token with "repo" scope.

You will only need to do this once.

## Publish the release

This script will download the binaries and generate the node headers and the .lib linker used on Windows by node-gyp to build native modules.

```sh
npm run release
```

Note: Many distributions of Python still ship with old HTTPS certificates. You may see a `InsecureRequestWarning`, but it can be disregarded.

## Delete the temporary branch

```sh
git checkout master
git branch -D release # delete local branch
git push origin :release # delete remote branch
```

[the releases page]: https://github.com/electron/electron/releases
[this bump commit]: https://github.com/electron/electron/commit/78ec1b8f89b3886b856377a1756a51617bc33f5a
[electron-versioning]: /docs/tutorial/electron-versioning.md

## Fix missing binaries of a release manually

In the case of a corrupted release with broken CI machines, we might have to
re-upload the binaries for an already published release.

The first step is to go to the
[Releases](https://github.com/electron/electron/releases) page and delete the
corrupted binaries with the `SHASUMS256.txt` checksum file.

Then manually create distributions for each platform and upload them:

```sh
# Checkout the version to re-upload.
git checkout vTHE.RELEASE.VERSION

# Do release build, specifying one target architecture.
./script/bootstrap.py --target_arch [arm|x64|ia32]
./script/build.py -c R
./script/create-dist.py

# Explicitly allow overwritting a published release.
./script/upload.py --overwrite
```

After re-uploading all distributions, publish again to upload the checksum
file:

```sh
npm run release
```

# Releasing

This document describes the process for releasing a new version of Electron.

## Set your tokens and environment variables
You'll need Electron S3 credentials in order to create and
upload an Electron release. Contact a team member for more
information.

There are a handful of `*_TOKEN` environment variables needed by the release
scripts. Once you've generated these per-user tokens, you may want to keep
them in a local file that you can `source` when starting a release.
* `ELECTRON_GITHUB_TOKEN`:
Create as described at https://github.com/settings/tokens/new,
giving the token repo access scope.
* `APPVEYOR_TOKEN`:
Create a token from https://windows-ci.electronjs.org/api-token
If you don't have an account, ask a team member to add you.
* `CIRCLE_TOKEN`:
Create a token from "Personal API Tokens" at https://circleci.com/account/api
* `VSTS_TOKEN`:
Create a Personal Access Token at https://github.visualstudio.com/_usersSettings/tokens
with the scope of `Build (read and execute)`.

## Determine which branch to release from

- **If releasing beta,** run the scripts below from `master`.
- **If releasing a stable version,** run the scripts below from the branch
  you're stabilizing.

## Find out what version change is needed
Run `npm run prepare-release -- --notesOnly` to view auto generated release
notes. The notes generated should help you determine if this is a major, minor,
patch, or beta version change. Read the
[Version Change Rules](../tutorial/electron-versioning.md#semver) for more information.

**NB:** If releasing from a branch, e.g. 1-8-x, check out the branch with
`git checkout 1-8-x` rather than `git checkout -b remotes/origin/1-8-x`.
The scripts need `git rev-parse --abbrev-ref HEAD` to return a short name,
e.g. no `remotes/origin/`

## Run the prepare-release script
The prepare release script will do the following:
1. Check if a release is already in process and if so it will halt.
2. Create a release branch.
3. Bump the version number in several files. See [this bump commit] for an example.
4. Create a draft release on GitHub with auto-generated release notes.
5. Push the release branch.
6. Call the APIs to run the release builds.

Once you have determined which type of version change is needed, run the
`prepare-release` script with arguments according to your need:
- `[major|minor|patch|beta]` to increment one of the version numbers, or
- `--stable` to indicate this is a stable version

For example:

### Major version change
```sh
npm run prepare-release -- major
```
### Minor version change
```sh
npm run prepare-release -- minor
```
### Patch version change
```sh
npm run prepare-release -- patch
```
### Beta version change
```sh
npm run prepare-release -- beta
```
### Promote beta to stable
```sh
npm run prepare-release -- --stable
```

Tip: You can test the new version number before running `prepare-release` with
a dry run of the `bump-version` script with the same major/minor/patch/beta
arguments, e.g.:
```sh
$ ./script/bump-version.py --bump minor --dry-run
```

## Wait for builds :hourglass_flowing_sand:
The `prepare-release` script will trigger the builds via API calls.
To monitor the build progress, see the following pages:

- [electron-release-mas-x64](https://github.visualstudio.com/electron/_build/index?context=allDefinitions&path=%5C&definitionId=19&_a=completed) for MAS builds.
- [electron-release-osx-x64](https://github.visualstudio.com/electron/_build/index?context=allDefinitions&path=%5C&definitionId=18&_a=completed) for OSX builds.
- [circleci.com/gh/electron/electron](https://circleci.com/gh/electron) for Linux builds.
- [windows-ci.electronjs.org/project/AppVeyor/electron-39ng6](https://windows-ci.electronjs.org/project/AppVeyor/electron-39ng6) for Windows 32-bit builds.
- [windows-ci.electronjs.org/project/AppVeyor/electron](https://windows-ci.electronjs.org/project/AppVeyor/electron) for Windows 64-bit builds.

## Compile release notes

Writing release notes is a good way to keep yourself busy while the builds are running.
For prior art, see existing releases on [the releases page].

Tips:
- Each listed item should reference a PR on electron/electron, not an issue, nor a PR from another repo like libcc.
- No need to use link markup when referencing PRs. Strings like `#123` will automatically be converted to links on github.com.
- To see the version of Chromium, V8, and Node in every version of Electron, visit [atom.io/download/electron/index.json](https://atom.io/download/electron/index.json).

### Patch releases

For a `patch` release, use the following format:

```sh
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

```sh
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
```sh
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
Use the same formats as the ones suggested above, but add the following note at
the beginning of the changelog:
```sh
**Note:** This is a beta release and most likely will have have some
instability and/or regressions.

Please file new issues for any bugs you find in it.

This release is published to [npm](https://www.npmjs.com/package/electron)
under the `beta` tag and can be installed via `npm install electron@beta`.
```


## Edit the release draft

1. Visit [the releases page] and you'll see a new draft release with placeholder
release notes.
2. Edit the release and add release notes.
3. Click 'Save draft'. **Do not click 'Publish release'!**
4. Wait for all builds to pass before proceeding.
5. In the branch, verify that the release's files have been created:
```sh
$ npm run release -- --validateRelease
```

## Publish the release

Once the merge has finished successfully, run the `release` script
via `npm run release` to finish the release process. This script will do the
following:
1. Build the project to validate that the correct version number is being released.
2. Download the binaries and generate the node headers and the .lib linker used
on Windows by node-gyp to build native modules.
3. Create and upload the SHASUMS files stored on S3 for the node files.
4. Create and upload the SHASUMS256.txt file stored on the GitHub release.
5. Validate that all of the required files are present on GitHub and S3 and have
the correct checksums as specified in the SHASUMS files.
6. Publish the release on GitHub

## Publish to npm

Before publishing to npm, you'll need to log into npm as Electron. Optionally,
you may find [npmrc](https://www.npmjs.com/package/npmrc) to be a useful way
to keep Electron's profile side-by-side with your own:
```sh
$ sudo npm install -g npmrc
$ npmrc -c electron
Removing old .npmrc (default)
Activating .npmrc "electron"
```

The Electron account's credentials are kept by GitHub.
"Electron - NPM" for the URL "https://www.npmjs.com/login".
```sh
$ npm login
Username: electron
Password:
Email: (this IS public) electron@github.com
```

Publish the release to npm.
```sh
$ npm whoami
electron
$ npm run publish-to-npm
```

[the releases page]: https://github.com/electron/electron/releases
[this bump commit]: https://github.com/electron/electron/commit/78ec1b8f89b3886b856377a1756a51617bc33f5a
[versioning]: /docs/tutorial/electron-versioning.md

# Troubleshooting

## Rerun broken builds

If a release build fails for some reason, you can use `script/ci-release-build.js` to rerun a release build:

### Rerun all linux builds:
```sh
node script/ci-release-build.js --ci=CircleCI --ghRelease TARGET_BRANCH
(TARGET_BRANCH) is the branch you are releasing from.
```

### Rerun all macOS builds:
```sh
node script/ci-release-build.js --ci=VSTS --ghRelease TARGET_BRANCH
(TARGET_BRANCH) is the branch you are releasing from.
```

### Rerun all Windows builds:
```sh
node script/ci-release-build.js --ci=AppVeyor --ghRelease TARGET_BRANCH
(TARGET_BRANCH) is the branch you are releasing from.
```

Additionally you can pass a job name to the script to run an individual job, eg:
````sh
node script/ci-release-build.js --ci=AppVeyor --ghRelease --job=electron-x64 TARGET_BRANCH
```

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

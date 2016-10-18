# Releasing

This document describes the process for releasing a new version of Electron.

## Compile release notes

The current process is to maintain a local file, keeping track of notable changes as pull requests are merged. For examples of how to format the notes, see previous releases on [the releases page].

## Create a temporary branch (optional)

If there is any change to the build configuration, use a temporary branch with any name (e.g. `release`). Otherwise you can use `master`.

## Bump the version

Run the `bump-release` script, passing `major`, `minor`, or `patch` as an argument:

```sh
npm run bump-release -- patch
```

This will bump the version number in a number of files. See [this bump commit] for an example.

Most releases will be `patch`-level. Upgrades to Chrome or other major changes should use `minor`. For more info, see [electron-versioning].

## Edit the release draft

1. Visit [the releases page] and you'll see a new draft release with placeholder release notes.
1. Edit the release and add release notes.
1. Click 'Save draft'. **Do not click 'Publish release'!**
1. Wait for all the builds to pass. :hourglass_flowing_sand:

## Merge temporary branch

If you created a temporary release branch, merge it back into master, without creating a merge commit:

```sh
git merge release master --no-commit
```

## Run local debug build

Run local debug build to verify that you are actually building the version you want. Sometimes you thought you were doing a release for a new version, but you're actually not.

```sh
npm run build
npm start
```

Verify the window is displaying the current updated version.

## Publish the release

This script will download the binaries and generate the node headers and the .lib linker used on Windows by node-gyp to build native modules.

```sh
npm run release
```

[the releases page]: https://github.com/electron/electron/releases
[this bump commit]: https://github.com/electron/electron/commit/78ec1b8f89b3886b856377a1756a51617bc33f5a
[electron-versioning]: /docs/tutorial/electron-versioning.md

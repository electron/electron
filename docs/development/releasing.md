# Releasing

This document describes the process for releasing a new version of Electron.

## Compile release notes

The current process is to maintain a local file, keeping track of notable changes as pull requests are merged. For examples of how to format the notes, see previous releases on [the releases page].

## Create a temporary branch

Create a new branch from `master` named `release`.

```sh
git checkout master
git pull
git checkout -b release
```

This branch is created as a precaution to prevent any merged PRs from sneaking into a release between the time the temporary release branch is created and the CI builds are complete.

## Bump the version

Run the `bump-version` script, passing `major`, `minor`, or `patch` as an argument:

```sh
npm run bump-version -- patch
```

This will bump the version number in several files. See [this bump commit] for an example.

Most releases will be `patch` level. Upgrades to Chrome or other major changes should use `minor`. For more info, see [electron-versioning].

## Edit the release draft

1. Visit [the releases page] and you'll see a new draft release with placeholder release notes.
1. Edit the release and add release notes.
1. Click 'Save draft'. **Do not click 'Publish release'!**
1. Wait for all the builds to pass. :hourglass_flowing_sand:

## Merge temporary branch

Merge the temporary back into master, without creating a merge commit:

```sh
git merge release master --no-commit
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

- `$ELECTRON_S3_BUCKET`
- `$ELECTRON_S3_ACCESS_KEY`
- `$ELECTRON_S3_SECRET_KEY`
- `$ELECTRON_GITHUB_TOKEN` - A personal access token with "repo" scope.

You will only need to do this once.

## Publish the release

This script will download the binaries and generate the node headers and the .lib linker used on Windows by node-gyp to build native modules.

```sh
npm run release
```

## Delete the temporary branch

```sh
git checkout master
git branch -D release # delete local branch
git push origin :release # delete remote branch
```

[the releases page]: https://github.com/electron/electron/releases
[this bump commit]: https://github.com/electron/electron/commit/78ec1b8f89b3886b856377a1756a51617bc33f5a
[electron-versioning]: /docs/tutorial/electron-versioning.md

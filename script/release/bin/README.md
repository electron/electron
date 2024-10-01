# Release Scripts

> These ancient artifacts date back to the early days of Electron, they have been modified
> over the years but in reality still very much look how they did at the beginning. You
> have been warned.

None of these scripts are called manually, they are each called by Sudowoodo at various points
in the Electron release process. What each script does though is loosely documented below,
however this documentation is a best effort so please be careful when modifying the scripts
as there still may be unknown or undocumented effects / intentions.

## What scripts do we have?

### `cleanup-release`

This script completely reverts a failed or otherwise unreleasable version. It does this by:

* Deleting the draft release if it exists
* Deleting the git tag if it exists

> [!NOTE]
> This is the only script / case where an existing tag will be deleted. Tags are only considered immutable after the release is published.

### `print-next-version`

This script just outputs the theoretical "next" version that a release would use.

### `prepare-for-release`

This script creates all the requisite tags and CI builds that will populate required release assets.

* Creates the git tag
* Kicks off all release builds on AppVeyor and GitHub Actions

### `run-release-build`

This script is used to re-kick specific release builds after they fail. Sudowoodo is responsible for prompting the release team as to whether or not to run this script. It's currently only used for AppVeyor builds.

> [!IMPORTANT]
> This script should be removed and the "rerun" logic for AppVeyor be implemented in Sudowoodo specifically in the same way that GitHub Actions' rerun logic is.

### `validate-before-publish`

This script ensures that a release is in a valid state before publishing it anywhere. Specifically it checks:

* All assets exist
* All checksums match uploaded assets
* Headers have been uploaded to the header CDN
* Symbols have been uploaded to the symbol CDN

### `publish-to-github`

This script finalizes the GitHub release, in the process it:

* Uploads the header SHASUMs to the CDN
* Updates the `index.json` file on the assets CDN with the new version via metadumper
* Publishes the actual GitHub release

### `publish-to-npm`

This script finishes the release process by publishing a new `npm` package.

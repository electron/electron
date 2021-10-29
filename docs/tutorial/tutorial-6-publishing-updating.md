---
title: 'Publishing and Updating'
description: "There are several ways to update an Electron application. The easiest and officially supported one is taking advantage of the built-in Squirrel framework and Electron's autoUpdater module."
slug: tutorial-publishing-updating
hide_title: false
---

:::info Tutorial parts
This is **part 6** of the Electron tutorial. The other parts are:

1. [Prerequisites][prerequisites]
1. [Scaffolding][scaffolding]
1. [Communicating Between Processes][main-renderer]
1. [Adding Features][features]
1. [Packaging Your Application][packaging]
1. [Publishing and Updating][updates]

:::

If you've been following along, this is the last step of the tutorial! In this part,
we'll publish your app to GitHub and integrate automatic updates
into your app code with update.electronjs.org.

Note that there are you need to meet certain criteria to use this service:

- App runs on macOS or Windows
- App has a public GitHub repository
- Builds are published to [GitHub Releases]
- Builds are [Code Signed][code-signed]

At this point, we'll assume that you've already committed and pushed all your
code to a public GitHub repository.

:::info Alternative update services
If you're using an alternate repository host (e.g. GitLab or Bitbucket) or if
you need to keep your code repository private, a [step-by-step guide][update-server] on
implementing your own Electron update server with Electron's [autoUpdater] API is
available.
:::

## Publishing a GitHub release

Electron Forge has [Publisher] plugins that can automate the distribution
of your packaged application to various sources. In this tutorial, we'll
be going over using the GitHub Publisher, which will allow us to publish
our code to GitHub Releases.

### Generating a personal access token

Forge cannot immediately publish to any repository on GitHub. You
need to pass in an authenticated token that gives Forge access to
your GitHub Releases. The easiest way to do this is to
[create a new personal access token (PAT)][new-pat]
with the `public_repo` scope, which gives write access to your public repositories.
**Make sure to keep this token a secret.**

We recommend you pass this as a `GITHUB_TOKEN` environment variable.

### Setting up the GitHub Publisher

Forge's [GitHub Publisher] is a plugin that
needs to be installed as a separate dev dependency:

```sh npm2yarn
npm install --save-dev @electron-forge/publisher-github
```

Once you have it installed, you need to set it up in your Forge
configuration. A full list of options is documented in the Forge's
[`PublisherGitHubConfig`] API docs.

```json title='package.json' {6-16}
{
  //...
  "config": {
    "forge": {
      "publishers": [
        {
          "name": "@electron-forge/publisher-github",
          "config": {
            "repository": {
              "owner": "github-user-name",
              "name": "github-repo-name"
            },
            "prerelease": false,
            "draft": true
          }
        }
      ]
    }
  }
  //...
}
```

:::tip Drafting releases before publishing
Notice that we're creating a draft of our release in this step.
This will allow you to see the release with its generated artifacts
without actually publishing it to your end users. You can manually
publish your releases via GitHub after writing release notes and
double-checking that your distributables work.
:::

You also need to make the Publisher aware of your authentication token.
We recommend you do so by setting it to the `GITHUB_TOKEN` environment
variable to avoid having to write it to your config file.

### Running the Publishing CLI command

At this point, you should be able to run Electron Forge's [Publish command],
which will run your Maker and publish the output distributables to a new
GitHub release.

```sh npm2yarn
npm run publish
```

By default, this will only publish a single distributable
for your host operating system and architecture. You can publish for
different architectures by passing in the `--arch` flag to your
Forge commands.

The name of this release will correspond to the `version` field
in your project's `package.json` file.

:::tip Git tip: tagging releases
Optionally, you can also [tag your releases in Git][git-tag] so that your
release is associated with a labeled point in your code history.
:::

#### Bonus: Publishing in GitHub Actions

Publishing locally can be painful, especially because you can only create distributables
for your host operating system (i.e. you can't publish a Window `.exe` file from macOS).

A solution for this would be to publish your app via automation workflows
such as [GitHub Actions], which can run tasks in the
cloud on Ubuntu, macOS, and Windows. This is the exact approach taken by the [Electron Fiddle] project.
You can refer to Fiddle's [Build and Release pipeline][fiddle-build]
and [Forge configuration][fiddle-forge-config]
for more details.

## Instrumenting your auto-updater code

Now that we have a functional release system via GitHub Releases, we now need to tell our
Electron app to download an update whenever a new release is out. Electron does this via
its [autoUpdater] module, which reads from an update server feed to check a new version
is available for download.

After your release is published to GitHub, the update.electronjs.org service should work
for your application. The only step left is to configure the feed with the autoUpdater module.

To make your life easier, the Electron team maintains the [update-electron-app] module,
which sets up the autoUpdater boilerplate for update.electronjs.org in one function
call — no configuration required.

First install the module as a **runtime dependency**.

```sh npm2yarn
npm install update-electron-app
```

Then, import the module and call it immediately in the main process.

```js title='main.js'
require('update-electron-app')();
```

And that's all it takes! Once your application is packaged, it will update itself for each new
GitHub Release that you publish.

[autoupdater]: ../api/auto-updater.md
[code-signed]: ./code-signing.md
[electron fiddle]: https://electronjs.org/fiddle
[fiddle-build]: https://github.com/electron/fiddle/blob/master/.github/workflows/build.yaml
[fiddle-forge-config]: https://github.com/electron/fiddle/blob/master/forge.config.js
[github actions]: https://github.com/features/actions
[github publisher]: https://www.electronforge.io/config/publishers/github
[github releases]: https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository
[new-pat]: https://github.com/settings/tokens/new
[publish command]: https://www.electronforge.io/cli#publish
[publisher]: https://www.electronforge.io/config/publishers
[`publishergithubconfig`]: https://js.electronforge.io/publisher/github/interfaces/publishergithubconfig
[update-electron-app]: https://github.com/electron/update-electron-app
[update-server]: ./updates.md#step-1-deploying-an-update-server

<!-- Tutorial links -->

[prerequisites]: tutorial-1-prerequisites.md
[scaffolding]: tutorial-2-scaffolding.md
[main-renderer]: tutorial-3-main-renderer.md
[features]: tutorial-4-adding-features.md
[packaging]: tutorial-5-packaging.md
[updates]: tutorial-6-publishing-updating.md

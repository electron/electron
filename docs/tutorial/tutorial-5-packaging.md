---
title: 'Packaging Your Application'
description: 'To distribute your app with Electron, you need to package it and create installers.'
slug: tutorial-packaging
hide_title: false
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

:::info Follow along the tutorial

This is **part 5** of the Electron tutorial.

1. [Prerequisites][prerequisites]
1. [Building your First App][building your first app]
1. [Using Preload Scripts][preload]
1. [Adding Features][features]
1. **[Packaging Your Application][packaging]**
1. [Publishing and Updating][updates]

:::

## Learning goals

In this part of the tutorial, we'll be going over the basics of packaging and distributing
your app with [Electron Forge][].

## Using Electron Forge

Electron does not have any tooling for packaging and distribution bundled into its core
modules. Once you have a working Electron app in dev mode, you need to use
additional tooling to create a packaged app you can distribute to your users (also known
as a **distributable**). Distributables can be either installers (e.g. MSI on Windows) or
portable executable files (e.g. `.app` on macOS).

Electron Forge is an all-in-one tool that handles the packaging and distribution of Electron
apps. Under the hood, it combines a lot of existing Electron tools (e.g. [`@electron/packager`][],
[`@electron/osx-sign`][], [`electron-winstaller`][], etc.) into a single interface so you do not
have to worry about wiring them all together.

### Importing your project into Forge

You can install Electron Forge's CLI in your project's `devDependencies` and import your
existing project with a handy conversion script.

```sh npm2yarn
npm install --save-dev @electron-forge/cli
npx electron-forge import
```

Once the conversion script is done, Forge should have added a few scripts
to your `package.json` file.

```json title='package.json'
  //...
  "scripts": {
    "start": "electron-forge start",
    "package": "electron-forge package",
    "make": "electron-forge make"
  },
  //...
```

:::info CLI documentation

For more information on `make` and other Forge APIs, check out
the [Electron Forge CLI documentation][].

:::

You should also notice that your package.json now has a few more packages installed
under `devDependencies`, and a new `forge.config.js` file that exports a configuration
object. You should see multiple makers (packages that generate distributable app bundles) in the
pre-populated configuration, one for each target platform.

### Creating a distributable

To create a distributable, use your project's new `make` script, which runs the
`electron-forge make` command.

```sh npm2yarn
npm run make
```

This `make` command contains two steps:

1. It will first run `electron-forge package` under the hood, which bundles your app
   code together with the Electron binary. The packaged code is generated into a folder.
1. It will then use this packaged app folder to create a separate distributable for each
   configured maker.

After the script runs, you should see an `out` folder containing both the distributable
and a folder containing the packaged application code.

```plain title='macOS output example'
out/
├── out/make/zip/darwin/x64/my-electron-app-darwin-x64-1.0.0.zip
├── ...
└── out/my-electron-app-darwin-x64/my-electron-app.app/Contents/MacOS/my-electron-app
```

The distributable in the `out/make` folder should be ready to launch! You have now
created your first bundled Electron application.

:::tip Distributable formats

Electron Forge can be configured to create distributables in different OS-specific formats
(e.g. DMG, deb, MSI, etc.). See Forge's [Makers][] documentation for all configuration options.

:::

:::tip Creating and adding application icons

Setting custom application icons requires a few additions to your config.
Check out [Forge's icon tutorial][] for more information.

:::

:::info Packaging without Electron Forge

If you want to manually package your code, or if you're just interested understanding the
mechanics behind packaging an Electron app, check out the full [Application Packaging][]
documentation.

:::

## Important: signing your code

In order to distribute desktop applications to end users, we _highly recommend_ that you **code sign** your Electron app. Code signing is an important part of shipping
desktop applications, and is mandatory for the auto-update step in the final part
of the tutorial.

Code signing is a security technology that you use to certify that a desktop app was
created by a known source. Windows and macOS have their own OS-specific code signing
systems that will make it difficult for users to download or launch unsigned applications.

On macOS, code signing is done at the app packaging level. On Windows, distributable installers
are signed instead. If you already have code signing certificates for Windows and macOS, you can set
your credentials in your Forge configuration.

:::info

For more information on code signing, check out the
[Signing macOS Apps](https://www.electronforge.io/guides/code-signing) guide in the Forge docs.

:::

<Tabs>
  <TabItem value="macos" label="macOS" default>

```js title='forge.config.js'
module.exports = {
  packagerConfig: {
    osxSign: {},
    // ...
    osxNotarize: {
      tool: 'notarytool',
      appleId: process.env.APPLE_ID,
      appleIdPassword: process.env.APPLE_PASSWORD,
      teamId: process.env.APPLE_TEAM_ID
    }
    // ...
  }
}
```

  </TabItem>
  <TabItem value="windows" label="Windows">

```js title='forge.config.js'
module.exports = {
  // ...
  makers: [
    {
      name: '@electron-forge/maker-squirrel',
      config: {
        certificateFile: './cert.pfx',
        certificatePassword: process.env.CERTIFICATE_PASSWORD
      }
    }
  ]
  // ...
}
```

  </TabItem>
</Tabs>

## Summary

Electron applications need to be packaged to be distributed to users. In this tutorial,
you imported your app into Electron Forge and configured it to package your app and
generate installers.

In order for your application to be trusted by the user's system, you need to digitally
certify that the distributable is authentic and untampered by code signing it. Your app
can be signed through Forge once you configure it to use your code signing certificate
information.

[`@electron/osx-sign`]: https://github.com/electron/osx-sign
[application packaging]: ./application-distribution.md
[`@electron/packager`]: https://github.com/electron/packager
[`electron-winstaller`]: https://github.com/electron/windows-installer
[electron forge]: https://www.electronforge.io
[electron forge cli documentation]: https://www.electronforge.io/cli#commands
[makers]: https://www.electronforge.io/config/makers
[forge's icon tutorial]: https://www.electronforge.io/guides/create-and-add-icons

<!-- Tutorial links -->

[prerequisites]: tutorial-1-prerequisites.md
[building your first app]: tutorial-2-first-app.md
[preload]: tutorial-3-preload.md
[features]: tutorial-4-adding-features.md
[packaging]: tutorial-5-packaging.md
[updates]: tutorial-6-publishing-updating.md

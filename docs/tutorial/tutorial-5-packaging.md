---
title: 'Packaging Your Application'
description: 'To distribute your app with Electron, you need to package and rebrand it. To do this, you can either use specialized tooling or manual approaches.'
slug: tutorial-packaging
hide_title: false
---

<!-- markdownlint-disable MD033 -->

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

:::info Tutorial parts
This is **part 5** of the Electron tutorial. The other parts are:

1. [Prerequisites][prerequisites]
1. [Scaffolding][scaffolding]
1. [Communicating Between Processes][main-renderer]
1. [Adding Features][features]
1. [Packaging Your Application][packaging]
1. [Publishing and Updating][updates]

:::

Electron does not have any tooling for packaging and distribution bundled into its core
modules. Once you have a working Electron app in development mode, you need to use
additional tooling to create a packaged app you can distribute to your users (also known
as a **distributable**). Distributables can be either installers (e.g. MSI on Windows) or
portable executable files (e.g. `.zip` on macOS).

In this section, we'll be going over the basics of packaging your app with
[Electron Forge].

## Importing your project into Electron Forge

First, add Electron Forge as a development dependency of your app and use its `import`
command to scaffold your existing app into a Forge project:

```sh npm2yarn
npm install --save-dev @electron-forge/cli
npx electron-forge import
```

Once the conversion script is done, Forge should have added a few scripts
to your `package.json` file.

```json title='package.json' {5}
  //...
  "scripts": {
    "start": "electron-forge start",
    "package": "electron-forge package",
    "make": "electron-forge make"
  },
  //...
```

:::info CLI documentation
For more information on `make` and other Forge commands, check out
the [Electron Forge CLI documentation].
:::

You should also notice that your `package.json` now has a few more packages installed
under your dev dependencies, and contains an added `config.forge` field with an array
of `makers` configured. **Makers** are Forge plugins that create distributables from
your source code. You should see multiple Makers in the default

## Creating a distributable

To create a distributable, use Forge's `make` script:

```sh npm2yarn
npm run make
```

After the script runs, you should see `out` folder containing both the packaged
application in folder format and the distributable:

```plain title='macOS output example'
out/
├── out/make/zip/darwin/x64/my-electron-app-darwin-x64-1.0.0.zip
├── ...
└── out/my-electron-app-darwin-x64/my-electron-app.app/Contents/MacOS/my-electron-app
```

The distributable in the `out/make` folder should be ready to launch! You have now
created your first full-fledged Electron application.

:::tip Distributable formats
Electron Forge can be configured to create distributables in different OS-specific formats
(e.g. DMG, deb, MSI, etc.). See Forge's [Makers] documentation for all configuration options.
:::

:::info Packaging without Electron Forge
If you want to manually package your code, or if you're just interested understanding the
mechanics behind packaging an Electron app, check out the full [Application Packaging]
guide.
:::

## Important: signing your code

In order to distribute desktop applications to end users, we _highly recommended_ for you
to **code sign** your Electron app.

Code signing is a security technology that you use to certify that a desktop app was
created by a known source. Windows and macOS have their own OS-specific code signing
systems that will make it difficult for users to download or launch unsigned applications.

If you already have code signing certificates for Windows and macOS, you can set your
credentials in your Forge configuration:

<Tabs>
  <TabItem value="macos" label="macOS" default>

```json title='package.json' {6-18}
{
  //...
  "config": {
    "forge": {
      //...
      "packagerConfig": {
        "osxSign": {
          "identity": "Developer ID Application: Felix Rieseberg (LT94ZKYDCJ)",
          "hardened-runtime": true,
          "entitlements": "entitlements.plist",
          "entitlements-inherit": "entitlements.plist",
          "signature-flags": "library"
        },
        "osxNotarize": {
          "appleId": "felix@felix.fun",
          "appleIdPassword": "my-apple-id-password"
        }
      }
      //...
    }
  }
  //...
}
```

  </TabItem>
  <TabItem value="windows" label="Windows">

```json title='package.json'  {6-14}
{
  //...
  "config": {
    "forge": {
      //...
      "makers": [
        {
          "name": "@electron-forge/maker-squirrel",
          "config": {
            "certificateFile": "./cert.pfx",
            "certificatePassword": "this-is-a-secret"
          }
        }
      ]
      //...
    }
  }
  //...
}
```

  </TabItem>
</Tabs>

If you're unfamiliar with the desktop application code signing process, please refer
to the full [Code Signing] guide for more information.

[Application Packaging]: application-packaging.md
[Code Signing]: code-signing.md
[Electron Forge]: https://www.electronforge.io
[Electron Forge CLI documentation]: https://www.electronforge.io/cli#commands
[Makers]: https://www.electronforge.io/config/makers

<!-- Tutorial links -->

[prerequisites]: tutorial-1-prerequisites.md
[scaffolding]: tutorial-2-scaffolding.md
[main-renderer]: tutorial-3-main-renderer.md
[features]: tutorial-4-adding-features.md
[packaging]: tutorial-5-packaging.md
[updates]: tutorial-6-publishing-updating.md

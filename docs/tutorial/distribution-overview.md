---
title: 'Distribution Overview'
description: 'To distribute your app with Electron, you need to package and rebrand it. To do this, you can either use specialized tooling or manual approaches.'
slug: distribution-overview
hide_title: false
---

Once your app is ready for production, there are a couple steps you need to take before
you can deliver it to your users.

## Packaging

To distribute your app with Electron, you need to package all your resources and assets
into an executable and rebrand it. To do this, you can either use specialized tooling like Electron Forge
or do it manually. See the [Application Packaging][application-packaging] tutorial
for more information.

## Code signing

Code signing is a security technology that you use to certify that an app was
created by you. You should sign your application so it does not trigger the
security checks of your user's operating system.

To get started with each operating system's code signing process, please read the
[Code Signing][code-signing] docs.

## Publishing

Once your app is packaged and signed, you can freely distribute your app directly
to users by uploading your installers online.

To reach more users, you can also choose to upload your app to each operating system's
digital distribution platform (i.e. app store). These require another build step aside
from your direct download app. For more information, check out each individual app store guide:

- [Mac App Store][mac-app]
- [Windows Store][windows-store]
- [Snapcraft (Linux)][snapcraft]

## Updating

Electron's auto-updater allows you to deliver application updates to users
without forcing them to manually download new versions of your application.
Check out the [Updating Applications][updates] guide for details on implementing automatic updates
with Electron.

<!-- Link labels -->

[application-packaging]: ./application-distribution.md
[code-signing]: ./code-signing.md
[mac-app]: ./mac-app-store-submission-guide.md
[windows-store]: ./windows-store-guide.md
[snapcraft]: ./snapcraft.md
[updates]: ./updates.md

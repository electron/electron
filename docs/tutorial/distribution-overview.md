---
title: 'Distribution Overview'
description: 'To distribute your app with Electron, you need to package and rebrand it. To do this, you can either use specialized tooling or manual approaches.'
slug: distribution-overview
hide_title: false
---


In order to share your application with your users, there are a few steps and
things you need to consider first. In this section we will see what are those
and provide links to other parts of the documentation to know more about them.

## Packaging

To distribute your app with Electron, you need to package all your resources
and assets into an executable, and rebrand it.
To do this, you can either use specialized tooling or manual approaches.

The [Application Packagin][application-packaging] section goes in detail about
the different techniques.

## Code signing

Code signing is a security technology that you use to certify that an app was
created by you. You should sign your application so it does not trigger the
security checks of your user's Operating System.

To know more about what to do for each Operating System, please read the
[Code Signing][code-signing] section.

## Publishing

Once you have the executable of your application signed, you can share it on
a file server and link to it from a website so your for your users can download
it (for example), or you can use the Operating System's store. Each platform
has its own caveats which are discussed in the following sections:

- [Mac App Store][mac-app]
- [Windows Store][windows-store]
- [Snapcraft (Linux)][snapcraft]

## Updating

If your plan on updating your application, you need a way to deliver those updates
without forcing the user to manually find and download the installer for the new
version. There are several ways to achieve this and the [Updating Applications][updates]
section explains them.

<!-- Link labels -->

[application-packaging]: ./application-packaging.md
[code-signing]: ./code-signing.md
[mac-app]: ./mac-app-store-submission-guide.md
[windows-store]: ./windows-store-guide.md
[snapcraft]: ./snapcraft.md
[updates]: ./updates.md

# Code Signing

Code signing is a security technology that you use to certify that an app was
created by you.

On macOS the system can detect any change to the app, whether the change is
introduced accidentally or by malicious code.

On Windows the system assigns a trust level to your code signing certificate which
if you don't have, or if your trust level is low will cause security dialogs to
appear when users start using your application.  Trust level builds over time
so it's better to start code signing as early as possible.

While it is possible to distribute unsigned apps, it is not recommended. Both
Windows and macOS will, by default, prevent either the download or the
execution of unsigned applications. Starting with macOS Catalina (version 10.15),
users have to go through multiple manual steps to open unsigned applications.

![macOS Catalina Gatekeeper warning: The app cannot be opened because the developer cannot be verified](../images/gatekeeper.png)

As you can see, users get two options: Move the app straight to the trash or
cancel running it. You don't want your users to see that dialog.

If you are building an Electron app that you intend to package and distribute,
it should be code-signed. The Mac and Windows app stores do not allow unsigned
apps.

# Signing macOS builds

Before signing macOS builds, you must do the following:

1. Enroll in the [Apple Developer Program] (requires an annual fee)
2. Download and install [Xcode]
3. Generate, download, and install [signing certificates]

There are a number of tools for signing your packaged app:

- [`electron-osx-sign`] is a standalone tool for signing macOS packages.
- [`electron-packager`] bundles `electron-osx-sign`. If you're using `electron-packager`,
pass the `--osx-sign=true` flag to sign your build.
  - [`electron-forge`] uses `electron-packager` internally, you can set the `osxSign` option
    in your forge config.
- [`electron-builder`] has built-in code-signing capabilities. See [electron.build/code-signing](https://www.electron.build/code-signing)

## Notarization

Starting with macOS Catalina, Apple requires applications to be notarized.
"Notarization" as defined by Apple means that you upload your previously signed
application to Apple for additional verification _before_ distributing the app
to your users.

To automate this process, you can use the [`electron-notarize`] module. You
do not necessarily need to complete this step for every build you make – just
the builds you intend to ship to users.

## Mac App Store

See the [Mac App Store Guide].

# Signing Windows builds

Before signing Windows builds, you must do the following:

1. Get a Windows Authenticode code signing certificate (requires an annual fee)
2. Install Visual Studio 2015/2017 (to get the signing utility)

You can get a code signing certificate from a lot of resellers. Prices vary, so it may be worth your time to shop around. Popular resellers include:

* [digicert](https://www.digicert.com/code-signing/microsoft-authenticode.htm)
* [Comodo](https://www.comodo.com/landing/ssl-certificate/authenticode-signature/)
* [GoDaddy](https://au.godaddy.com/web-security/code-signing-certificate)
* Amongst others, please shop around to find one that suits your needs, Google is your friend :)

There are a number of tools for signing your packaged app:

- [`electron-winstaller`] will generate an installer for windows and sign it for you
- [`electron-forge`] can sign installers it generates through the Squirrel.Windows or MSI targets.
- [`electron-builder`] can sign some of its windows targets

## Windows Store

See the [Windows Store Guide].

[Apple Developer Program]: https://developer.apple.com/programs/
[`electron-builder`]: https://github.com/electron-userland/electron-builder
[`electron-forge`]: https://github.com/electron-userland/electron-forge
[`electron-osx-sign`]: https://github.com/electron-userland/electron-osx-sign
[`electron-packager`]: https://github.com/electron/electron-packager
[`electron-notarize`]: https://github.com/electron/electron-notarize
[`electron-winstaller`]: https://github.com/electron/windows-installer
[Xcode]: https://developer.apple.com/xcode
[signing certificates]: https://github.com/electron/electron-osx-sign/wiki/1.-Getting-Started#certificates
[Mac App Store Guide]: mac-app-store-submission-guide.md
[Windows Store Guide]: windows-store-guide.md

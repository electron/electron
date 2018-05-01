# Code Signing

Code signing is a security technology that you use to certify that an app was 
created by you. Once an app is signed, the system can detect any change to the 
app, whether the change is introduced accidentally or by malicious code.

While it is possible to distribute unsigned apps, it is not recommended. 
For example, here's what macOS users see when attempting to start an unsigned app:

![unsigned app warning on macOS](https://user-images.githubusercontent.com/2289/39488937-bdc854ba-4d38-11e8-88f8-7b3c125baefc.png)

> App can't be opened because it is from an unidentified developer

If you are building an Electron app that you intend to package and distribute, 
it should be code signed. The Mac and Windows app stores do not allow unsigned 
apps.

# Signing macOS builds

Before signing macOS builds, you must do the following:

1. Enroll in the [Apple Developer Program](Apple Developer Program) (requires an annual fee)
2. Download and install Xcode
3. Generate, download, and install [signing certificates]

There are a number of tools for signing your packaged app:

- [`electron-osx-sign`] is a standalone tool for signing macOS packages.
- [`electron-packager`] bundles `electron-osx-sign`. If you're using `electron-packager`,
pass the `--osx-sign=true` flag to sign your build.
- [`electron-builder`] has built-in code-signing capabilities. See [electron.build/code-signing](https://www.electron.build/code-signing)

For more info, see the [Mac App Store Submission Guide].

# Signing Windows builds

See the [Windows Store Guide].

[Apple Developer Program]: https://developer.apple.com/programs/
[`electron-osx-sign`]: https://github.com/electron-userland/electron-osx-sign
[`electron-packager`]: https://github.com/electron-userland/electron-packager
[`electron-builder`]: https://github.com/electron/electron-builder
[Xcode]: https://developer.apple.com/xcode
[signing certificates]: https://github.com/electron-userland/electron-osx-sign/wiki/1.-Getting-Started#certificates
[Mac App Store Submission Guide]: mac-app-store-submission-guide.md
[Windows Store Guide]: windows-store-guide.md
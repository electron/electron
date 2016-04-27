# Application Distribution

To distribute your app with Electron, the folder containing your app should be
named `app` and placed under Electron's resources directory (on OS X it is
`Electron.app/Contents/Resources/` and on Linux and Windows it is `resources/`),
like this:

On OS X:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

On Windows and Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Then execute `Electron.app` (or `electron` on Linux, `electron.exe` on Windows),
and Electron will start as your app. The `electron` directory will then be
your distribution to deliver to final users.

## Packaging Your App into a File

Apart from shipping your app by copying all of its source files, you can also
package your app into an [asar](https://github.com/atom/asar) archive to avoid
exposing your app's source code to users.

To use an `asar` archive to replace the `app` folder, you need to rename the
archive to `app.asar`, and put it under Electron's resources directory like
below, and Electron will then try to read the archive and start from it.

On OS X:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

On Windows and Linux:

```text
electron/resources/
└── app.asar
```

More details can be found in [Application packaging](application-packaging.md).

## Rebranding with Downloaded Binaries

After bundling your app into Electron, you will want to rebrand Electron
before distributing it to users.

### Windows

You can rename `electron.exe` to any name you like, and edit its icon and other
information with tools like [rcedit](https://github.com/atom/rcedit).

### OS X

You can rename `Electron.app` to any name you want, and you also have to rename
the `CFBundleDisplayName`, `CFBundleIdentifier` and `CFBundleName` fields in
following files:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

You can also rename the helper app to avoid showing `Electron Helper` in the
Activity Monitor, but make sure you have renamed the helper app's executable
file's name.

The structure of a renamed app would be like:

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

You can rename the `electron` executable to any name you like.

## Rebranding by Rebuilding Electron from Source

It is also possible to rebrand Electron by changing the product name and
building it from source. To do this you need to modify the `atom.gyp` file and
have a clean rebuild.

### grunt-build-atom-shell

Manually checking out Electron's code and rebuilding could be complicated, so
a Grunt task has been created that will handle this automatically:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

This task will automatically handle editing the `.gyp` file, building from
source, then rebuilding your app's native Node modules to match the new
executable name.

## Packaging Tools

Apart from packaging your app manually, you can also choose to use third party
packaging tools to do the work for you:

* [electron-packager](https://github.com/maxogden/electron-packager)
* [electron-builder](https://github.com/loopline-systems/electron-builder)

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
package your app into an [asar](https://github.com/electron/asar) archive to avoid
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

### Creating a Custom Electron Fork

Creating a custom fork of Electron is almost certainly not something you will
need to do in order to build your app, even for "Production Level" applications.
Using a tool such as `electron-packager` or `electron-builder` will allow you to
"Rebrand" Electron without having to do these steps.

You need to fork Electron when you have custom C++ code that you have patched
directly into Electron, that either cannot be upstreamed, or has been rejected
from the official version. As maintainers of Electron, we very much would like
to make your scenario work, so please try as hard as you can to get your changes
into the official version of Electron, it will be much much easier on you, and
we want to help you out!

#### Creating a Full Custom Electron Release with surf-build

1. Install [Surf](https://github.com/surf-build/surf), via npm:
  `npm install -g surf-build@latest`

1. Create a new S3 bucket and create the following empty directory structure:

```
- atom-shell
  - symbols
  - dist
```

1. Set the following Environment Variables:

  * `ATOM_SHELL_GITHUB_TOKEN` - a token that can create releases on GitHub
  * `ATOM_SHELL_S3_ACCESS_KEY`, `ATOM_SHELL_S3_BUCKET`, `ATOM_SHELL_S3_SECRET_KEY`
    - the place where you'll upload node.js headers as well as symbols
  * `ELECTRON_RELEASE` - Set to `true` and the upload part will run, leave unset
    and `surf-build` will just do CI-type checks, appropriate to run for every
    pull request.
  * `CI` - Set to `true` or else we'll fail
  * `GITHUB_TOKEN` - set it to the same as `ATOM_SHELL_GITHUB_TOKEN`
  * `SURF_TEMP` - set to `C:\Temp` on Windows to prevent path too long issues
  * `TARGET_ARCH` - set to `ia32` or `x64`  

1. In `script/upload.py`, you _must_ set `ATOM_SHELL_REPO` to your fork,
  especially if you are a contributor to Electron proper.

1. `surf-build -r https://github.com/MYORG/electron -s YOUR_COMMIT -n 'surf-PLATFORM-ARCH'`

1. Wait a very, very long time.

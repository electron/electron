# Application distribution

To distribute your app with atom-shell, you should name the folder of your app
as `app`, and put it under atom-shell's resources directory (on OS X it is
`Atom.app/Contents/Resources/`, and on Linux and Windows it is `resources/`),
like this:

On Mac OS X:

```text
atom-shell/Atom.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

On Windows and Linux:

```text
atom-shell/resources/app
├── package.json
├── main.js
└── index.html
```

Then execute `Atom.app` (or `atom` on Linux, and `atom.exe` on Windows), and
atom-shell will start as your app. The `atom-shell` directory would then be
your distribution that should be delivered to final users.

## Packaging your app into a file

Apart from shipping your app by copying all its sources files, you can also
package your app into an [asar](https://github.com/atom/asar) archive to avoid
exposing your app's source code to users.

To use an `asar` archive to replace the `app` folder, you need to rename the
archive to `app.asar`, and put it under atom-shell's resources directory like
bellow, and atom-shell will then try read the archive and start from it.

On Mac OS X:

```text
atom-shell/Atom.app/Contents/Resources/
└── app.asar
```

On Windows and Linux:

```text
atom-shell/resources/
└── app.asar
```

More details can be found in [Application packaging](application-packaging.md).

## Renaming atom-shell for your app

### Renaming by rebuilding

The best way to rename atom-shell is to change the `atom.gyp` file, then build
from source. Open up `atom.gyp` and change the two lines:

```
'project_name': 'atom',
'product_name': 'Atom',
```

Once you make the change, re-run `script/bootstrap.py` then run the command:

```sh
script/build.py -c Release -t whatever_you_chose_for_project_name
```

### Renaming with grunt-build-atom-shell

Manually checking out atom-shell's code and rebuilding could be complicated, so
a Grunt task has been created that will handle this automatically,
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

This task will automatically handle editing the `.gyp` file, building from
source, then rebuilding your app's native Node modules to match the new
executable name.

### Renaming the downloaded binaries

If you don't care about the executable name on Windows or the helper process
name on OS X, you can simply rename the downloaded binaries, and there is also a
grunt task that can download atom-shell for your current platform automatically,
[grunt-download-atom-shell](https://github.com/atom/grunt-download-atom-shell).


#### Windows

You can not rename the `atom.exe` otherwise native modules will not load. But
you can edit the executable's icon and other information with tools like
[rcedit](https://github.com/atom/rcedit) or [ResEdit](http://www.resedit.net).

#### OS X

You can rename `Atom.app` to whatever you want, and you also have to rename the
`CFBundleDisplayName`, `CFBundleIdentifier` and `CFBundleName` fields in
following manifest files if they have the keys:

* `Atom.app/Contents/Info.plist`
* `Atom.app/Contents/Frameworks/Atom Helper.app/Contents/Info.plist`

#### Linux

You can rename the `atom` executable to whatever you want.

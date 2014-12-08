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

## Renaming Atom Shell for your app

The best way to rename Atom Shell is to change the `atom.gyp` file, then build
from source. Open up `atom.gyp` and change the first lines:

```
'project_name': 'atom',
'product_name': 'Atom',
'framework_name': 'Atom Framework',
```

Once you make the change, re-run `script/bootstrap` then run the command:

```sh
script/build.py -c Release -t $whatever_you_chose_for_project_name
```

If your app is OS X / Linux-only, you can also simply rename the "Atom.app"
folder as well as the names under "Framework" (i.e. "Atom Framework.framework"
=> "MyApp Framework.framework"), but this will break loading native Node
modules on Windows.

Fixing this is complicated, but a Grunt task has been created that will handle
this automatically,
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).
This task will automatically handle editing the .gyp file, building from
source, then rebuilding your app's native Node modules to match the new
executable name.

## Packaging your app into a file

Apart from shipping your app by copying all its sources files, you can also
package your app into an [asar](https://github.com/atom/asar) archive to avoid
exposing your app's source code to users.

To use an `asar` archive to replace the `app` folder, you need to rename the
archive to `app.asar`, and put it under atom-shell's resources directory,
atom-shell will then try read the archive and start from it. 

More details can be found in [Application packaging](application-packaging.md).

## Building with grunt

If you build your application with `grunt` there is a grunt task that can
download atom-shell for your current platform automatically:
[grunt-download-atom-shell](https://github.com/atom/grunt-download-atom-shell).

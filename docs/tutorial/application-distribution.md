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

Then execute `Atom.app` (or `atom` on Linux, and `atom.exe` on Window), and
atom-shell will start as your app. The `atom-shell` directory would then be
your distribution that should be delivered to final users.

## Build with grunt

If you build your application with `grunt` there is a grunt task that can
download atom-shell for your current platform automatically:
[grunt-download-atom-shell](https://github.com/atom/grunt-download-atom-shell).

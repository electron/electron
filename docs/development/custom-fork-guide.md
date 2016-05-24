## Do I need to do this? Almost certainly not!

Creating a custom fork of Electron is almost certainly not something you will need to do in order to build your app, even for "Production Level" applications. Using a tool such as `electron-packager` or `electron-builder` will allow you to "Rebrand" Electron without having to do these steps.

You need to fork Electron when you have custom C++ code that you have patched directly into Electron, that either cannot be upstreamed, or has been rejected from the official version. As maintainers of Electron, we very much would like to make your scenario work, so please try as hard as you can to get your changes into the official version of Electron, it will be much much easier on you, and we want to help you out!

If you're still convinced, press on to...

## How to create a full custom release for Electron with surf-build

1. Install [Surf](https://github.com/surf-build/surf), via npm: `npm install -g surf-build@latest`

1. Create a new S3 bucket and create the following empty directory structure:

```
- atom-shell
  - symbols
  - dist
```

1. Set the following Environment Variables:

  * `ATOM_SHELL_GITHUB_TOKEN` - a token that can create releases on GitHub
  * `ATOM_SHELL_S3_ACCESS_KEY`, `ATOM_SHELL_S3_BUCKET`, `ATOM_SHELL_S3_SECRET_KEY` - the place where you'll upload node.js headers as well as symbols
  * `ELECTRON_RELEASE` - Set to `true` and the upload part will run, leave unset and `surf-build` will just do CI-type checks, appropriate to run for every PR.
  * `CI` - Set to `true` or else we'll fail
  * `GITHUB_TOKEN` - set it to the same as `ATOM_SHELL_GITHUB_TOKEN`
  * `SURF_TEMP` - set to `C:\Temp` on Windows or else you'll have MAX_PATH hatin'
  * `TARGET_ARCH` - set to `ia32` or `x64`  

1. In `script/upload.py`, you _must_ set `ATOM_SHELL_REPO` to your fork, especially if you are a contributor to Electron proper

1. `surf-build -r https://github.com/MYORG/electron -s YOUR_COMMIT -n 'surf-PLATFORM-ARCH'`

1. Wait a very, very long time.


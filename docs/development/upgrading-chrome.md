# Upgrading Chromium Workflow

This document is meant to serve as an overview of what steps are needed
on each Chromium upgrade in Electron.

- Upgrade libcc to a new Chromium version
- Make Electron code compatible with the new libcc
- Update Electron dependencies (crashpad, NodeJS, etc.)  if needed
- Make internal builds of libcc and electron
- Update Electron docs if necessary


## Upgrade `libcc` to a new Chromium

### Steps
1. Get the code and initialize the project:
  - ```
     $ git clone git@github.com:electron/libchromiumcontent.git
     $ cd libchromiumcontent
     $ ./script/bootstrap -v
    ```
2. Get a new beta/stable Chromium revision from OmahaProxy
3. Put it into the VERSION file, then run `$ ./script/update`
 - It will probably fail applying patches.
4. Fix `*.patch` files in the `/patches` and `/patches-mas` folders.
5. (Optional) Run a separate script to apply patches (`script/update` uses it internally):
 - `$ ./script/apply-patches`
 - There is also another script `/script/patch.py` that could be more useful, check `--help` to learn how it works with `$ ./script/patch.py -h`
6. Run the build when all patches can be applied without errors
 `$ ./script/build`
 - If some patches are no longer compatible with the Chromium code, fix compilation errors.
7. When build succeeds, create a `dist` for Electron
 `$ ./script/create-dist  --no_zip`
 - It will create `dist/main` folder in the root of the libcc repo, you will need it to build Electron.
8. (Optional) Update script contents if there are errors resultant of some files being removed or renamed. (`--no_zip` prevents script from create `dist` archives, you don't need them.)


## Update Electron Code

### Steps
1. Get the code:
  - ```
    $ git clone git@github.com:electron/electron.git
    $ cd electron
    ```
2. If you already have libcc built on you machine in its own repo, you need to tell Electron explicitly to use it:
  - ```
   $ ./script/bootstrap.py -v \
  	--libcc_source_path <libcc_folder>/src \
  	--libcc_shared_library_path <libcc_folder>/shared_library \
  	--libcc_static_library_path <libcc_folder>/static_library
  ```
  - If you haven't yet built libcc but it's already supposed to be upgraded to a new Chromium, bootstrap Electron as usual
    - `$ ./script/bootstrap.py -v`
    - Ensure that libcc submodule (`vendor/libchromiumcontent`) points to a right revision

3. Set CLANG_REVISION in` script/update-clang.sh` to match the version Chromium is using.
  - Located in `electron/libchromiumcontent/src/tools/clang/scripts/update.py`

4. Checkout Chromium if you haven't already: https://chromium.googlesource.com/chromium/src.git/+/{VERSION}/tools/clang/scripts/update.py
  - (Replace the `{VERSION}` placeholder in the url above to the Chromium version libcc uses.)
5. Build Electron.
  - Try to build Debug version first: `$ ./script/build.py -c D`
  - You will need it to run tests
6. Fix compilation and linking errors
7. Ensure that Release build can be built too: `$ ./script/build.py -c R`
  - Often you will have different linking errors in Release build, fix them too.
  -Some compilation and linking errors are caused by missing source/object files in the libcc `dist`
8. Update `./script/create-dist` in the libcc repo, recreate a `dist`, and run Electron bootstrap script once again.

#### Tips for fixing compilation errors
- Fix build config errors first
- Fix fatal errors first, like missing files and errors related to compiler flags or defines
- Try to identify complex errors as soon as possible,
  - Ask for help if you're not sure how to fix them
- Disable all Electron features, fix the build, then enable them one by one
- Add more build flags to disable features in build-time.

When Electron successfully builds in a Debug configuration, try to run tests:
`$ ./script/test.py`
Fix the failing tests.

Follow all the steps above to fix Electron code on all supported platforms.


## Update Crashpad

- Electron's crashpad fork: https://github.com/electron/crashpad
- Primary crashpad repo: https://chromium.googlesource.com/crashpad/crashpad/


### Steps
If there are any compilation errors related to the Crashpad, it probably means you need to update the fork to a newer revision:

1. Clone Electron's fork of the Crashpad, add the main repo as another remote:
  - ```$ git clone https://github.com/electron/crashpad && cd crashpad
  $ git remote add upstream https://chromium.googlesource.com/crashpad/crashpad/ && git fetch upstream
  ```
2. Find a revision Chromium uses in src/third_party/crashpad/README.chromium
3. Rebase the master branch to that REVISION
  - `$ git rebase REVISION`
  - If there are nontrivial conflicts during rebase, consult with the commit's author.
4. Add a tag with the next Electron version to a top commit in the master branch
  - e.g. if current Electron version is `1.11.*`, then use tag `electron-1.12.0`, because it will be used in the next version.
5. Ensure Electron-specific commits in the master branch on GitHub are properly tagged
  - If they aren't, force push will make them orphans and they'll be lost.
6. Force push to Electron's crashpad fork.
7. Update `/vendor/crashpad` submodule in the Electron's upgrade branch to point to the updated crashpad.


## Update NodeJS

- Upgrade `vendor/node` to the Node release that corresponds to the v8 version
  being used in the new Chromium release. See the v8 versions in Node on
  https://nodejs.org/en/download/releases for more details.
  - You can find v8 version Chromium is using on [OmahaProxy](http://omahaproxy.appspot.com).
    If it's not available check `v8/include/v8-version.h`
    in the Chromium checkout.

## Verify ffmpeg Support

Electron ships with a version of `ffmpeg` that includes proprietary codecs by
default. A version without these codecs is built and distributed with each
release as well. Each Chrome upgrade should verify that switching this version is
still supported.

You can verify Electron's support for multiple `ffmpeg` builds by loading the
following page. It should work with the default `ffmpeg` library distributed
with Electron and not work with the `ffmpeg` library built without proprietary
codecs.

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Proprietary Codec Check</title>
  </head>
  <body>
    <p>Checking if Electron is using proprietary codecs by loading video from http://www.quirksmode.org/html5/videos/big_buck_bunny.mp4</p>
    <p id="outcome"></p>
    <video style="display:none" src="http://www.quirksmode.org/html5/videos/big_buck_bunny.mp4" autoplay></video>
    <script>
      const video = document.querySelector('video')
      video.addEventListener('error', ({target}) => {
        if (target.error.code === target.error.MEDIA_ERR_SRC_NOT_SUPPORTED) {
          document.querySelector('#outcome').textContent = 'Not using proprietary codecs, video emitted source not supported error event.'
        } else {
          document.querySelector('#outcome').textContent = `Unexpected error: ${target.error.code}`
        }
      })
      video.addEventListener('playing', () => {
        document.querySelector('#outcome').textContent = 'Using proprietary codecs, video started playing.'
      })
    </script>
  </body>
</html>
```

## Useful Links

- [Chrome Release Schedule](https://www.chromium.org/developers/calendar)
- [OmahaProxy](http://omahaproxy.appspot.com)
- [Chromium Issue Tracker](https://bugs.chromium.org/p/chromium)

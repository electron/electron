# Upgrading Chromium

This is an overview of the steps needed to upgrade Chromium in Electron.

- Upgrade libcc to a new Chromium version
- Make Electron code compatible with the new libcc
- Update Electron dependencies (crashpad, NodeJS, etc.) if needed
- Make internal builds of libcc and electron
- Update Electron docs if necessary


## Upgrade `libcc` to a new Chromium version

1. Get the code and initialize the project:
  ```sh
  $ git clone git@github.com:electron/libchromiumcontent.git
  $ cd libchromiumcontent
  $ ./script/bootstrap -v
  ```
2. Update the Chromium snapshot
  - Choose a version number from [OmahaProxy](https://omahaproxy.appspot.com/)
    and update the `VERSION` file with it
    - This can be done manually by visiting OmahaProxy in a browser, or automatically:
    - One-liner for the latest stable mac version: `curl -so- https://omahaproxy.appspot.com/mac > VERSION`
    - One-liner for the latest win64 beta version: `curl -so- https://omahaproxy.appspot.com/all | grep "win64,beta" | awk -F, 'NR==1{print $3}' > VERSION`
  - run `$ ./script/update`
    - Brew some tea -- this may run for 30m or more.
    - It will probably fail applying patches.
3. Fix `*.patch` files in the `patches/` and `patches-mas/` folders.
4. (Optional) `script/update` applies patches, but if multiple tries are needed
   you can manually run the same script that `update` calls:
   `$ ./script/apply-patches`
  - There is a second script, `script/patch.py` that may be useful.
    Read `./script/patch.py -h` for more information.
5. Run the build when all patches can be applied without errors
  - `$ ./script/build`
  - If some patches are no longer compatible with the Chromium code,
    fix compilation errors.
6. When the build succeeds, create a `dist` for Electron
  - `$ ./script/create-dist --no_zip`
    - It will create a `dist/main` folder in the libcc repo's root.
      You will need this to build Electron.
7. (Optional) Update script contents if there are errors resulting from files
   that were removed or renamed. (`--no_zip` prevents script from create `dist`
   archives. You don't need them.)


## Update Electron's code

1. Get the code:
  ```sh
  $ git clone git@github.com:electron/electron.git
  $ cd electron
  ```
2. If you have libcc built on your machine in its own repo,
   tell Electron to use it:
  ```sh
  $ ./script/bootstrap.py -v \
    --libcc_source_path <libcc_folder>/src \
    --libcc_shared_library_path <libcc_folder>/shared_library \
    --libcc_static_library_path <libcc_folder>/static_library
  ```
3. If you haven't yet built libcc but it's already supposed to be upgraded
   to a new Chromium, bootstrap Electron as usual
   `$ ./script/bootstrap.py -v`
  - Ensure that libcc submodule (`vendor/libchromiumcontent`) points to the
    right revision

4. Set `CLANG_REVISION` in `script/update-clang.sh` to match the version
   Chromium is using.
  - Located in `electron/libchromiumcontent/src/tools/clang/scripts/update.py`

5. Checkout Chromium if you haven't already:
  - https://chromium.googlesource.com/chromium/src.git/+/{VERSION}/tools/clang/scripts/update.py
    - (Replace the `{VERSION}` placeholder in the url above to the Chromium
      version libcc uses.)
6. Build Electron.
  - Try to build Debug version first: `$ ./script/build.py -c D`
  - You will need it to run tests
7. Fix compilation and linking errors
8. Ensure that Release build can be built too
  - `$ ./script/build.py -c R`
  - Often the Release build will have different linking errors that you'll
    need to fix.
  - Some compilation and linking errors are caused by missing source/object
    files in the libcc `dist`
9. Update `./script/create-dist` in the libcc repo, recreate a `dist`, and
   run Electron bootstrap script once again.

### Tips for fixing compilation errors
- Fix build config errors first
- Fix fatal errors first, like missing files and errors related to compiler
  flags or defines
- Try to identify complex errors as soon as possible.
  - Ask for help if you're not sure how to fix them
- Disable all Electron features, fix the build, then enable them one by one
- Add more build flags to disable features in build-time.

When a Debug build of Electron succeeds, run the tests:
`$ ./script/test.py`
Fix the failing tests.

Follow all the steps above to fix Electron code on all supported platforms.


## Updating Crashpad

If there are any compilation errors related to the Crashpad, it probably means
you need to update the fork to a newer revision. See
[Upgrading Crashpad](upgrading-crashpad.md)
for instructions on how to do that.


## Updating NodeJS

Upgrade `vendor/node` to the Node release that corresponds to the v8 version
used in the new Chromium release. See the v8 versions in Node on

See [Upgrading Node](upgrading-node.md)
for instructions on this.

## Verify ffmpeg support

Electron ships with a version of `ffmpeg` that includes proprietary codecs by
default. A version without these codecs is built and distributed with each
release as well. Each Chrome upgrade should verify that switching this version
is still supported.

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

## Useful links

- [Chrome Release Schedule](https://www.chromium.org/developers/calendar)
- [OmahaProxy](http://omahaproxy.appspot.com)
- [Chromium Issue Tracker](https://bugs.chromium.org/p/chromium)

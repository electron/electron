# Upgrading Chromium Workflow

This document is meant to serve as an overview of what steps are needed
on each Chromium upgrade in Electron.

## Update libchromiumcontent (a.k.a. libcc)

- Clone the repo:
```sh
git clone git@github.com:electron/libchromiumcontent.git
cd libchromiumcontent
```
- Run bootstrap script to init and sync git submodules:
```sh
./script/bootstrap -v
```
- Update `VERSION` file to correspond to Chromium version.
- Run `script/update`, it will probably fail applying patches.
- Fix failing patches. `script/patch.py` might help.
  - Don't forget to fix patches in the `patches-mas/` folder.
- Build libcc:
```sh
./script/build
```
- Create dist folders which will be used by electron:
```sh
./script/create-dist --no_zip
cd dist/main
../../tools/generate_filenames_gypi filenames.gypi src shared_library static_library
cd -
```
- Open a pull request to `electron/libchromiumcontent` with the changes.
- Fix compilation on the all supported platforms/arches.

## Update Electron

- Set `vendor/libchromiumcontent` revision to a version with the new Chromium.
  - It will be great if GH builds for this libcc version are already green
    and its archives are already available. Otherwise everyone would need
    to build libcc locally in order to try build a new Electron.
- Set `CLANG_REVISION` in `script/update-clang.sh` to match the version
  Chromium is using. You can find it in file `src/tools/clang/scripts/update.py` in updated `electron/libchromiumcontent` repo.
- Run `script/bootstrap.py`.
- Upgrade Node.js if you are willing to. See the notes below.
- Fix compilation.
- Open a pull request on `electron/electron` with the changes.
  - This should include upgrading the submodules in `vendor/` as needed.
- Fix failing tests.

## Upgrade Node.js

- Upgrade `vendor/node` to the Node release that corresponds to the v8 version
  being used in the new Chromium release. See the v8 versions in Node on
  https://nodejs.org/en/download/releases for more details.
  - You can find v8 version Chromium is using on [OmahaProxy](http://omahaproxy.appspot.com).
    If it's not available check `v8/include/v8-version.h`
    in the Chromium checkout.

## Troubleshooting

**TODO**


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

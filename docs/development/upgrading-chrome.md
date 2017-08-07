# Upgrading Chromium Workflow

This document is meant to serve as an overview of what steps are needed
on each Chromium upgrade in Electron.

## Update libchromiumcontent (a.k.a. libcc)

- Update the `VERSION` file at the root of the `electron/libchromiumcontent`
  repository.
- Run `script/update`, it will probably fail applying patches.
- Fix failing patches. `script/patch.py` might help.
  - Don't forget to fix patches in the `patches-mas/` folder.
- Make sure compilation succeeds on at least one supported platform/arch.
- Open a pull request on `electron/libchromiumcontent` with the changes.
- Fix compilation on the all supported platforms/arches.

## Fix Electron code

- Set `vendor/libchromiumcontent` revision to a version with the new Chromium.
  - It will be great if GH builds for this libcc version are already green
    and its archives are already available. Otherwise everyone would need
    to build libcc locally in order to try build a new Electron.
- Set `CLANG_REVISION` in `script/update-clang.sh` to match the version
  Chromium is using in `tools/clang/scripts/update.py`.
  If you don't have a Chromium checkout you can find it there:
  https://chromium.googlesource.com/chromium/src.git/+/{VERSION}/tools/clang/scripts/update.py
  (Just change the `{VERSION}` in the URL to the Chromium version you need.)
- Don't forget to (re)run `script/bootstrap.py`.
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


==OLD STUFF==

# Upgrading Chrome Checklist
This document is meant to serve as an overview of what steps are needed
on each Chrome upgrade in Electron.

These are things to do in addition to updating the Electron code for any
Chrome/Node API changes.

- Verify the new Chrome version is available from
  https://github.com/zcbenz/chromium-source-tarball/releases
- Update the `VERSION` file at the root of the `electron/libchromiumcontent`
  repository
- Update the `CLANG_REVISION` in `script/update-clang.sh` to match the version
  Chrome is using in `libchromiumcontent/src/tools/clang/scripts/update.py`
- Upgrade `vendor/node` to the Node release that corresponds to the v8 version
  being used in the new Chrome release. See the v8 versions in Node on
  https://nodejs.org/en/download/releases for more details
- Upgrade `vendor/crashpad` for any crash reporter changes needed
- Upgrade `vendor/depot_tools` for any build tools changes needed
- Update the `libchromiumcontent` SHA-1 to download in `script/lib/config.py`
- Open a pull request on `electron/libchromiumcontent` with the changes
- Open a pull request on `electron/electron` with the changes
  - This should include upgrading the submodules in `vendor/` as needed
- Verify debug builds succeed on:
  - macOS
  - 32-bit Windows
  - 64-bit Window
  - 32-bit Linux
  - 64-bit Linux
  - ARM Linux
- Verify release builds succeed on:
  - macOS
  - 32-bit Windows
  - 64-bit Window
  - 32-bit Linux
  - 64-bit Linux
  - ARM Linux
- Verify tests pass on:
  - macOS
  - 32-bit Windows
  - 64-bit Window
  - 32-bit Linux
  - 64-bit Linux
  - ARM Linux

==/OLD STUFF==


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

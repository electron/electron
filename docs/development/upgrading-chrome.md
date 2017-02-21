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
- Open a pull request on `electron/brightray` with the changes
  - This should include upgrading the `vendor/libchromiumcontent` submodule
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

## Links

- [Chrome Release Schedule](https://www.chromium.org/developers/calendar)

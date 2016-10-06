# Build System Overview

Electron uses [gyp](https://gyp.gsrc.io/) for project generation and
[ninja](https://ninja-build.org/) for building. Project configurations can
be found in the `.gyp` and `.gypi` files.

## Gyp Files

Following `gyp` files contain the main rules for building Electron:

* `electron.gyp` defines how Electron itself is built.
* `common.gypi` adjusts the build configurations of Node to make it build
  together with Chromium.
* `vendor/brightray/brightray.gyp` defines how `brightray` is built and
  includes the default configurations for linking with Chromium.
* `vendor/brightray/brightray.gypi` includes general build configurations about
  building.

## Component Build

Since Chromium is quite a large project, the final linking stage can take
quite a few minutes, which makes it hard for development. In order to solve
this, Chromium introduced the "component build", which builds each component as
a separate shared library, making linking very quick but sacrificing file size
and performance.

In Electron we took a very similar approach: for `Debug` builds, the binary
will be linked to a shared library version of Chromium's components to achieve
fast linking time; for `Release` builds, the binary will be linked to the static
library versions, so we can have the best possible binary size and performance.

## Minimal Bootstrapping

All of Chromium's prebuilt binaries (`libchromiumcontent`) are downloaded when
running the bootstrap script. By default both static libraries and shared
libraries will be downloaded and the final size should be between 800MB and 2GB
depending on the platform.

By default, `libchromiumcontent` is downloaded from Amazon Web Services.
If the `LIBCHROMIUMCONTENT_MIRROR` environment variable is set, the bootstrap
script will download from it.
[`libchromiumcontent-qiniu-mirror`](https://github.com/hokein/libchromiumcontent-qiniu-mirror)
is a mirror for `libchromiumcontent`. If you have trouble in accessing AWS, you
can switch the download address to it via
`export LIBCHROMIUMCONTENT_MIRROR=http://7xk3d2.dl1.z0.glb.clouddn.com/`

If you only want to build Electron quickly for testing or development, you
can download just the shared library versions by passing the `--dev` parameter:

```bash
$ ./script/bootstrap.py --dev
$ ./script/build.py -c D
```

## Two-Phase Project Generation

Electron links with different sets of libraries in `Release` and `Debug`
builds. `gyp`, however, doesn't support configuring different link settings for
different configurations.

To work around this Electron uses a `gyp` variable
`libchromiumcontent_component` to control which link settings to use and only
generates one target when running `gyp`.

## Target Names

Unlike most projects that use `Release` and `Debug` as target names, Electron
uses `R` and `D` instead. This is because `gyp` randomly crashes if there is
only one `Release` or `Debug` build configuration defined, and Electron only has
to generate one target at a time as stated above.

This only affects developers, if you are just building Electron for rebranding
you are not affected.

## Tests

Test your changes conform to the project coding style using:

```bash
$ npm run lint
```

Test functionality using:

```bash
$ npm test
```

Whenever you make changes to Electron source code, you'll need to re-run the
build before the tests:

```bash
$ npm run build && npm test
```

You can make the test suite run faster by isolating the specific test or block
you're currently working on using Mocha's
[exclusive tests](https://mochajs.org/#exclusive-tests) feature:

```js
describe.only('some feature', function () {
  // ... only tests in this block will be run
})
```

Alternatively, you can use mocha's `grep` option to only run tests matching the
given regular expression pattern:

```sh
$ npm test -- --grep child_process
```

Tests that include native modules (e.g. `runas`) can't be executed with the
debug build (see [#2558](https://github.com/electron/electron/issues/2558) for
details), but they will work with the release build.

To run the tests with the release build use:

```bash
$ npm test -- -R
```

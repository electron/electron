# Build System Overview

Electron uses [GN](https://gn.googlesource.com/gn) for project generation and
[ninja](https://ninja-build.org/) for building. Project configurations can
be found in the `.gn` and `.gni` files.

## GN Files

The following `gn` files contain the main rules for building Electron:

* `BUILD.gn` defines how Electron itself is built and
  includes the default configurations for linking with Chromium.
* `build/args/{debug,release,all}.gn` contain the default build arguments for
  building Electron.

## Component Build

Since Chromium is quite a large project, the final linking stage can take
quite a few minutes, which makes it hard for development. In order to solve
this, Chromium introduced the "component build", which builds each component as
a separate shared library, making linking very quick but sacrificing file size
and performance.

Electron inherits this build option from Chromium. In `Debug` builds, the
binary will be linked to a shared library version of Chromium's components to
achieve fast linking time; for `Release` builds, the binary will be linked to
the static library versions, so we can have the best possible binary size and
performance.

## Tests

**NB** _this section is out of date and contains information that is no longer
relevant to the GN-built electron._

Test your changes conform to the project coding style using:

```sh
$ npm run lint
```

Test functionality using:

```sh
$ npm test
```

Whenever you make changes to Electron source code, you'll need to re-run the
build before the tests:

```sh
$ npm run build && npm test
```

You can make the test suite run faster by isolating the specific test or block
you're currently working on using Mocha's
[exclusive tests](https://mochajs.org/#exclusive-tests) feature. Append
`.only` to any `describe` or `it` function call:

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

```sh
$ npm test -- -R
```

# Build system overview

Electron uses `gyp` for project generation, and `ninja` for building, project
configurations can be found in `.gyp` and `.gypi` files.

## Gyp files

Following `gyp` files contain the main rules of building Electron:

* `atom.gyp` defines how Electron itself is built.
* `common.gypi` adjusts the build configurations of Node to make it build
  together with Chromium.
* `vendor/brightray/brightray.gyp` defines how `brightray` is built, and
  includes the default configurations of linking with Chromium.
* `vendor/brightray/brightray.gypi` includes general build configurations about
  building.

## Component build

Since Chromium is quite a large project, the final linking stage would take
quite a few minutes, making it hard for development. In order to solve this,
Chromium introduced the "component build", which builds each component as a
separate shared library, making linking very quick but sacrificing file size
and performance.

In Electron we took a very similar approach: for `Debug` builds, the binary
will be linked to shared library version of Chromium's components to achieve
fast linking time; for `Release` builds, the binary will be linked to the static
library versions, so we can have the best possible binary size and performance.

## Minimal bootstrapping

All of Chromium's prebuilt binaries are downloaded when running the bootstrap
script. By default both static libraries and shared libraries will be
downloaded and the final size should be between 800MB and 2GB according to the
platform.

If you only want to build Electron quickly for testing or development, you
can only download the shared library versions by passing the `--dev` parameter:

```bash
$ ./script/bootstrap.py --dev
$ ./script/build.py -c D
```

## Two-phrase project generation

Electron links with different sets of libraries in `Release` and `Debug`
builds, however `gyp` doesn't support configuring different link settings for
different configurations.

To work around this Electron uses a `gyp` variable
`libchromiumcontent_component` to control which link settings to use, and only
generates one target when running `gyp`.

## Target names

Unlike most projects that use `Release` and `Debug` as target names, Electron
uses `R` and `D` instead. This is because `gyp` randomly crashes if there is
only one `Release` or `Debug` build configuration is defined, and Electron has
to only generate one target for one time as stated above.

This only affects developers, if you are only building Electron for rebranding
you are not affected.

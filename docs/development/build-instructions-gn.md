# Build Instructions (experimental GN build)

Follow the guidelines below for building Electron with the experimental GN
build.

> **NOTE**: The GN build system is in _experimental_ status, and currently only
> works on macOS and Linux, in debug mode, as a component build.

## Prerequisites

See the [macOS](build-instructions-osx.md#prerequisites) or
[Linux](build-instructions-linux.md#prerequisites) build instructions for the
requirements for your platform. In addition, you'll need to install
[`depot_tools`][depot-tools], the toolset used for fetching Chromium and its
dependencies.

[depot-tools]: http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up

## Getting the Code

```sh
$ mkdir electron-gn && cd electron-gn
$ cat > .gclient <<-GCLIENT
solutions = [
  {
    "url": "https://github.com/electron/electron",
    "managed": False,
    "name": "src/electron",
  },
]
GCLIENT
$ gclient sync --with_branch_heads --with_tags
# This will take a while, go get a coffee.
```

## Building

```sh
$ cd src
$ export CHROMIUM_BUILDTOOLS_PATH=`pwd`/buildtools
$ gn gen out/Default --args='root_extra_deps=["//electron"] is_electron_build=true is_component_build=true use_jumbo_build=true v8_promise_internal_field_count=1 v8_typed_array_max_size_in_heap=0'
```

This will generate all the ninja files needed for the build. You shouldn't have
to run `gn gen` again—if you want to change the build arguments you can run `gn
args out/Default` to bring up an editor.

To build, run `ninja` with the `electron:electron_app` target:

```sh
$ ninja -C out/Default electron:electron_app
# This will also take a while and probably heat up your lap.
```

This will build all of what was previously 'libchromiumcontent' (i.e. the
`content/` directory of `chromium` and its dependencies, incl. WebKit and V8),
so it will take a while.

To speed up subsequent builds, you can use [sccache][sccache]. Add the GN arg
`cc_wrapper="sccache"` by running `gn args out/Default` to bring up an editor.

[sccache]: https://github.com/mozilla/sccache

The built executable will be under `./out/Default`:

```sh
$ ./out/Default/Electron.app/Contents/MacOS/Electron
# or, on Linux
$ ./out/Default/electron
```

## Tests

To run the tests, you'll first need to build the test modules against the
same version of node.js that was built as part of the build process.

```sh
$ (cd electron/spec && npm i --nodedir=../../third_party/electron_node)
```

Then, run Electron with `electron/spec` as the argument:

```sh
$ ./out/Default/Electron.app/Contents/MacOS/Electron electron/spec
```

If you're debugging something, it can be helpful to pass some extra flags to
the Electron binary:

```sh
$ ./out/Default/Electron.app/Contents/MacOS/Electron electron/spec \
  --ci --enable-logging -g 'BrowserWindow module'
```

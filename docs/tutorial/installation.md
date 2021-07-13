# Installation

To install prebuilt Electron binaries, use [`npm`][npm].
The preferred method is to install Electron as a development dependency in your
app:

```sh
npm install electron --save-dev
```

See the [Electron versioning doc][versioning] for info on how to
manage Electron versions in your apps.

## Running Electron ad-hoc

If you're in a pinch and would prefer to not use `npm install` in your local
project, you can also run Electron ad-hoc using the [`npx`][npx] command runner
bundled with `npm`:

```sh
npx electron .
```

The above command will run the current working directory with Electron. Note that
any dependencies in your app will not be installed.

## Customization

If you want to change the architecture that is downloaded (e.g., `ia32` on an
`x64` machine), you can use the `--arch` flag with npm install or set the
`npm_config_arch` environment variable:

```shell
npm install --arch=ia32 electron
```

In addition to changing the architecture, you can also specify the platform
(e.g., `win32`, `linux`, etc.) using the `--platform` flag:

```shell
npm install --platform=win32 electron
```

## Proxies

If you need to use an HTTP proxy, you need to set the `ELECTRON_GET_USE_PROXY` variable to any
value, plus additional environment variables depending on your host system's Node version:

* [Node 10 and above][proxy-env-10]
* [Before Node 10][proxy-env]

## Custom Mirrors and Caches

During installation, the `electron` module will call out to
[`@electron/get`][electron-get] to download prebuilt binaries of
Electron for your platform. It will do so by contacting GitHub's
release download page (`https://github.com/electron/electron/releases/tag/v$VERSION`,
where `$VERSION` is the exact version of Electron).

If you are unable to access GitHub or you need to provide a custom build, you
can do so by either providing a mirror or an existing cache directory.

#### Mirror

You can use environment variables to override the base URL, the path at which to
look for Electron binaries, and the binary filename. The URL used by `@electron/get`
is composed as follows:

```javascript
url = ELECTRON_MIRROR + ELECTRON_CUSTOM_DIR + '/' + ELECTRON_CUSTOM_FILENAME
```

For instance, to use the China CDN mirror:

```shell
ELECTRON_MIRROR="https://cdn.npm.taobao.org/dist/electron/"
```

By default, `ELECTRON_CUSTOM_DIR` is set to `v$VERSION`. To change the format,
use the `{{ version }}` placeholder. For example, `version-{{ version }}`
resolves to `version-5.0.0`, `{{ version }}` resolves to `5.0.0`, and
`v{{ version }}` is equivalent to the default. As a more concrete example, to
use the China non-CDN mirror:

```shell
ELECTRON_MIRROR="https://npm.taobao.org/mirrors/electron/"
ELECTRON_CUSTOM_DIR="{{ version }}"
```

The above configuration will download from URLs such as
`https://npm.taobao.org/mirrors/electron/8.0.0/electron-v8.0.0-linux-x64.zip`.

#### Cache

Alternatively, you can override the local cache. `@electron/get` will cache
downloaded binaries in a local directory to not stress your network. You can use
that cache folder to provide custom builds of Electron or to avoid making contact
with the network at all.

* Linux: `$XDG_CACHE_HOME` or `~/.cache/electron/`
* macOS: `~/Library/Caches/electron/`
* Windows: `$LOCALAPPDATA/electron/Cache` or `~/AppData/Local/electron/Cache/`

On environments that have been using older versions of Electron, you might find the
cache also in `~/.electron`.

You can also override the local cache location by providing a `electron_config_cache`
environment variable.

The cache contains the version's official zip file as well as a checksum, stored as
a text file. A typical cache might look like this:

```sh
├── httpsgithub.comelectronelectronreleasesdownloadv1.7.9electron-v1.7.9-darwin-x64.zip
│   └── electron-v1.7.9-darwin-x64.zip
├── httpsgithub.comelectronelectronreleasesdownloadv1.7.9SHASUMS256.txt
│   └── SHASUMS256.txt
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.1electron-v1.8.1-darwin-x64.zip
│   └── electron-v1.8.1-darwin-x64.zip
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.1SHASUMS256.txt
│   └── SHASUMS256.txt
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.2-beta.1electron-v1.8.2-beta.1-darwin-x64.zip
│   └── electron-v1.8.2-beta.1-darwin-x64.zip
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.2-beta.1SHASUMS256.txt
│   └── SHASUMS256.txt
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.2-beta.2electron-v1.8.2-beta.2-darwin-x64.zip
│   └── electron-v1.8.2-beta.2-darwin-x64.zip
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.2-beta.2SHASUMS256.txt
│   └── SHASUMS256.txt
├── httpsgithub.comelectronelectronreleasesdownloadv1.8.2-beta.3electron-v1.8.2-beta.3-darwin-x64.zip
│   └── electron-v1.8.2-beta.3-darwin-x64.zip
└── httpsgithub.comelectronelectronreleasesdownloadv1.8.2-beta.3SHASUMS256.txt
    └── SHASUMS256.txt
```

## Skip binary download

Under the hood, Electron's JavaScript API binds to a binary that contains its
implementations. Because this binary is crucial to the function of any Electron app,
it is downloaded by default in the `postinstall` step every time you install `electron`
from the npm registry.

However, if you want to install your project's dependencies but don't need to use
Electron functionality, you can set the `ELECTRON_SKIP_BINARY_DOWNLOAD` environment
variable to prevent the binary from being downloaded. For instance, this feature can
be useful in continuous integration environments when running unit tests that mock
out the `electron` module.

```sh npm2yarn
ELECTRON_SKIP_BINARY_DOWNLOAD=1 npm install
```

## Troubleshooting

When running `npm install electron`, some users occasionally encounter
installation errors.

In almost all cases, these errors are the result of network problems and not
actual issues with the `electron` npm package. Errors like `ELIFECYCLE`,
`EAI_AGAIN`, `ECONNRESET`, and `ETIMEDOUT` are all indications of such
network problems. The best resolution is to try switching networks, or
wait a bit and try installing again.

You can also attempt to download Electron directly from
[electron/electron/releases][releases]
if installing via `npm` is failing.

If installation fails with an `EACCESS` error you may need to
[fix your npm permissions][npm-permissions].

If the above error persists, the [unsafe-perm][unsafe-perm] flag may need to be
set to true:

```sh
sudo npm install electron --unsafe-perm=true
```

On slower networks, it may be advisable to use the `--verbose` flag in order to
show download progress:

```sh
npm install --verbose electron
```

If you need to force a re-download of the asset and the SHASUM file set the
`force_no_cache` environment variable to `true`.

[npm]: https://docs.npmjs.com
[versioning]: ./electron-versioning.md
[npx]: https://docs.npmjs.com/cli/v7/commands/npx
[releases]: https://github.com/electron/electron/releases
[proxy-env-10]: https://github.com/gajus/global-agent/blob/v2.1.5/README.md#environment-variables
[proxy-env]: https://github.com/np-maintain/global-tunnel/blob/v2.7.1/README.md#auto-config
[electron-get]: https://github.com/electron/get
[npm-permissions]: https://docs.npmjs.com/getting-started/fixing-npm-permissions
[unsafe-perm]: https://docs.npmjs.com/misc/config#unsafe-perm

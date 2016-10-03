# Brightray

Brightray is a static library that makes
[libchromiumcontent](https://github.com/electron/libchromiumcontent) easier to
use in applications.

## Using it in your app

See [electron](https://github.com/electron/electron) for example of an
application written using Brightray.

## Development

### Prerequisites

* Python 2.7
* Linux:
    * Clang 3.0
* Mac:
    * Xcode
* Windows:
    * Visual Studio 2010 SP1

### One-time setup

You must previously have built and uploaded libchromiumcontent using its
`script/upload` script.

    $ script/bootstrap http://base.url.com/used/by/script/upload

### Building

    $ script/build

Building Brightray on its own isn’t all that interesting, since it’s just a
static library. Building it into an application is the only way to test it.

## License

In general, everything is covered by the [`LICENSE`](LICENSE) file. Some files
specify at the top that they are covered by the
[`LICENSE-CHROMIUM`](LICENSE-CHROMIUM) file instead.

# Brightray

Brightray is a static library that makes
[libchromiumcontent](https://github.com/brightray/libchromiumcontent) easier to
use in applications.

## Using it in your app

See [brightray_example](https://github.com/brightray/brightray_example) for a
sample application written using Brightray.

## Development

### Prerequisites

* Python 2.7
* gyp
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
static library. Building it into an application (like
[brightray_example](https://github.com/brightray/brightray_example)) is the only
way to test it.

## License

In general, everything is covered by the [`LICENSE`](LICENSE) file. Some files
specify at the top that they are covered by the
[`LICENSE-CHROMIUM`](LICENSE-CHROMIUM) file instead.

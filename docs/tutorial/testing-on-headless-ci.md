# Testing on Headless CI Systems (Travis CI, Jenkins)

## Overview

Being based on Chromium, Electron requires a display driver to function.
If Electron can't find a display driver, it will fail to launch and
will not execute any of your tests. Therefore, testing Electron-based apps
on headless continuous integration servers such as Travis CI, CircleCI,
or Jenkins requires a little bit of configuration.

## Configuring a Virtual Display Server

On your Linux machine, install [Xvfb](https://linux.die.net/man/1/xvfb),
a virtual framebuffer implementing the X11 display server protocol.
It performs all graphical operations in memory without showing any screen output,
which is exactly what we need.

Then, create a virtual Xvfb screen and export an environment variable
called `$DISPLAY` that points to it. Electron will automatically look
for `$DISPLAY`, so no further configuration of your app is required.

We recommend using the [xvfb-maybe](https://github.com/anaisbetts/xvfb-maybe)
package to automate the above step. Prepend your test commands with `xvfb-maybe`
and this test command will automatically configure Xvfb if required by the current system.
On Windows or macOS, it will do nothing.

```sh
## On Windows or macOS, this invokes electron-mocha
## On Linux, if we are in a headless environment, this will be equivalent
## to xvfb-run electron-mocha ./test/*.js
xvfb-maybe electron-mocha ./test/*.js
```

### TravisCI

On TravisCI, your `.travis.yml` should look roughly like this:

```yml
addons:
  apt:
    packages:
      - xvfb

install:
  - export DISPLAY=':99.0'
  - Xvfb :99 -screen 0 1024x768x24 > /dev/null 2>&1 &
```

### Jenkins

For Jenkins, an [Xvfb plugin is available](https://plugins.jenkins.io/xvfb/).

### Circle CI

Circle CI has Xvfb and the `$DISPLAY` variable
[already set up](https://circleci.com/docs/2.0/browser-testing/), so no
further configuration is required.

### AppVeyor

AppVeyor runs on Windows, which supports Selenium, Chromium, and Electron
tools out of the box. No configuration is required.

# atom-shell

[![build status](http://img.shields.io/travis/mafintosh/atom-shell.svg?style=flat)](http://travis-ci.org/mafintosh/atom-shell)

Install [atom-shell](https://github.com/atom/atom-shell) prebuilt binaries for command-line use using npm.

Works on Mac, Windows and Linux OSes that Atom Shell supports (e.g. Atom Shell [does not support Windows XP](https://github.com/atom/atom-shell/issues/691)).

Atom Shell is a javascript runtime that bundles Node.js and Chromium. You use it similar to the `node` command on the command line for executing javascript programs. This module helps you easily install the `atom-shell` command for use on the command line without having to compile anything.

## Installation

Download and install the latest build of atom-shell for your OS and symlink it into your PATH:

```
npm install -g atom-shell
```

If that command fails with an `EACCESS` error you may have to run it again with `sudo`:

```
sudo npm install -g atom-shell
```

Now you can just run `atom-shell` to run atom-shell:

```
atom-shell
```

## Usage

First you have to [write an atom shell application](https://github.com/atom/atom-shell/blob/master/docs/tutorial/quick-start.md#write-your-first-atom-shell-app)

Then you can run your app using:

```
atom-shell your-app/
```

## Programmatic usage

If you require `atom-shell` inside your node app it will return the file path to the binary.
Use this to spawn atom shell

``` js
var atom = require('atom-shell')
var proc = require('child_process')

// will something similar to print /Users/maf/.../Atom
console.log(atom)

// spawn atom-shell
var child = proc.spawn(atom)
```
# Developer Environment

Electron development is essentially Node.js development. To turn your operating
system into an environment capable of building desktop apps with Electron,
you will merely need Node.js, npm, a code editor of your choice, and a
rudimentary understanding of your operating system's command line client.

## Setting up macOS

> Electron supports Mac OS X 10.9 (and all versions named macOS) and up. Apple
does not allow running macOS in virtual machines unless the host computer is
already an Apple computer, so if you find yourself in need of a Mac, consider
using a cloud service that rents access to Macs (like [MacInCloud][macincloud]
or [xcloud](https://xcloud.me)).

First, install a recent version of Node.js. We recommend that you install
either the latest `LTS` or `Current` version available. Visit
[the Node.js download page][node-download] and select the `macOS Installer`.
While Homebrew is an offered option, but we recommend against it - many tools
will be incompatible with the way Homebrew installs Node.js.

Once downloaded, execute the installer and let the installation wizard guide
you through the installation.

Once installed, confirm that everything works as expected. Find the macOS
`Terminal` application in your `/Applications/Utilities` folder (or by
searching for the word `Terminal` in Spotlight). Open up `Terminal`
or another command line client of your choice and confirm that both `node`
and `npm` are available:

```sh
# This command should print the version of Node.js
node -v

# This command should print the version of npm
npm -v
```

If both commands printed a version number, you are all set! Before you get
started, you might want to install a [code editor](#a-good-editor) suited
for JavaScript development.

## Setting up Windows

> Electron supports Windows 7 and later versions – attempting to develop Electron
applications on earlier versions of Windows will not work. Microsoft provides
free [virtual machine images with Windows 10][windows-vm] for developers.

First, install a recent version of Node.js. We recommend that you install
either the latest `LTS` or `Current` version available. Visit
[the Node.js download page][node-download] and select the `Windows Installer`.
Once downloaded, execute the installer and let the installation wizard guide
you through the installation.

On the screen that allows you to configure the installation, make sure to
select the `Node.js runtime`, `npm package manager`, and `Add to PATH`
options.

Once installed, confirm that everything works as expected. Find the Windows
PowerShell by opening the Start Menu and typing `PowerShell`. Open
up `PowerShell` or another command line client of your choice and confirm that
both `node` and `npm` are available:

```powershell
# This command should print the version of Node.js
node -v

# This command should print the version of npm
npm -v
```

If both commands printed a version number, you are all set! Before you get
started, you might want to install a [code editor](#a-good-editor) suited
for JavaScript development.

## Setting up Linux

> Generally speaking, Electron supports Ubuntu 12.04, Fedora 21, Debian 8
and later.

First, install a recent version of Node.js. Depending on your Linux
distribution, the installation steps might differ. Assuming that you normally
install software using a package manager like `apt` or `pacman`, use the
official [Node.js guidance on installing on Linux][node-package].

You're running Linux, so you likely already know how to operate a command line
client. Open up your favorite client and confirm that both `node` and `npm`
are available globally:

```sh
# This command should print the version of Node.js
node -v

# This command should print the version of npm
npm -v
```

If both commands printed a version number, you are all set! Before you get
started, you might want to install a [code editor](#a-good-editor) suited
for JavaScript development.

## A Good Editor

We might suggest two free popular editors built in Electron:
GitHub's [Atom][atom] and Microsoft's [Visual Studio Code][code]. Both of
them have excellent JavaScript support.

If you are one of the many developers with a strong preference, know that
virtually all code editors and IDEs these days support JavaScript.

[macincloud]: https://www.macincloud.com/
[xcloud]: https://xcloud.me
[node-download]: https://nodejs.org/en/download/
[node-package]: https://nodejs.org/en/download/package-manager/
[atom]: https://atom.io/
[code]: https://code.visualstudio.com/
[windows-vm]: https://developer.microsoft.com/en-us/windows/downloads/virtual-machines

# Build instructions (Linux)

## Prerequisites

* [Node.js](http://nodejs.org)
* Clang 3.4 or later
* Development headers of GTK+ and libnotify

On Ubuntu you could install the libraries via:

```bash
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       gcc-multilib g++-multilib
```

Latest Node.js could be installed via ppa:

```bash
$ sudo apt-get install python-software-properties software-properties-common
$ sudo add-apt-repository ppa:chris-lea/node.js
$ sudo apt-get update
$ sudo apt-get install nodejs

# Update to latest npm
$ sudo npm install npm -g
```

## Getting the code

```bash
$ git clone https://github.com/atom/electron.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
build project files. Notice that we're using `ninja` to build Electron so
there is no `Makefile` generated.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## Building

Build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

You can also only build the `Debug` target:

```bash
$ ./script/build.py -c D
```

After building is done, you can find `atom` under `out/D`.

## Troubleshooting

### fatal error: bits/predefs.h: No such file or directory

If you got an error like this:

````
In file included from /usr/include/stdio.h:28:0,
                 from ../../../svnsrc/libgcc/../gcc/tsystem.h:88,
                 from ../../../svnsrc/libgcc/libgcc2.c:29:
/usr/include/features.h:324:26: fatal error: bits/predefs.h: No such file or directory
 #include <bits/predefs.h>
````

Then you need to install `gcc-multilib` and `g++-multilib`, on Ubuntu you can do
this:

```bash
$ sudo apt-get install gcc-multilib g++-multilib
```

### error adding symbols: DSO missing from command line

If you got an error like this:

````
/usr/bin/ld: vendor/download/libchromiumcontent/Release/libchromiumcontent.so: undefined reference to symbol 'gconf_client_get'
//usr/lib/x86_64-linux-gnu/libgconf-2.so.4: error adding symbols: DSO missing from command line
````

libchromiumcontent.so is build with clang 3.0 which is incompatible with newer
versions of clang. Try using clang 3.0, default version in Ubuntu 12.04.

### libudev.so.0 missing

If you get an error like:

````
/usr/bin/ld: warning: libudev.so.0, needed by .../vendor/brightray/vendor/download/libchromiumcontent/Release/libchromiumcontent.so, not found (try using -rpath or -rpath-link)
````

and you are on Ubuntu 13.04+, 64 bit system, try doing

```bash
sudo ln -s /lib/x86_64-linux-gnu/libudev.so.1.3.5 /usr/lib/libudev.so.0
```

for ubuntu 13.04+ 32 bit systems, try doing

```bash
sudo ln -s /lib/i386-linux-gnu/libudev.so.1.3.5  /usr/lib/libudev.so.0
```

also see

https://github.com/nwjs/nw.js/wiki/The-solution-of-lacking-libudev.so.0

## Tests

```bash
$ ./script/test.py
```

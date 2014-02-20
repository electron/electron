# Build instructions (Linux)

## Prerequisites

* [node.js](http://nodejs.org)
* clang

## Getting the code

```bash
$ git clone https://github.com/atom/atom-shell.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
build project files. Notice that we're using `ninja` to build `atom-shell` so
there is no `Makefile` generated.

```bash
$ cd atom-shell
$ ./script/bootstrap.py
```

## Building

Build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

You can also only build the `Debug` target:

```bash
$ ./script/build.py -c Debug
```

After building is done, you can find `Atom.app` under `out/Debug`.

## Troubleshooting

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
sudo apt-get install gcc-multilib g++-multilib
```

## Tests

```bash
$ ./script/test.py
```


# Build instructions (Linux)

## Prerequisites

* Python 2.7.x
* [Node.js] (http://nodejs.org)
* Clang 3.4 or later
* Development headers of GTK+ and libnotify

On Ubuntu you could install the libraries via:

```bash
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 gcc-multilib g++-multilib
```

## If You Use Virtual Machines For Building

If you plan to build electron on a virtual machine, you will need a fixed-size
device container of at least 25 gigabytes in size. The default disk sizes
suggested by VirtualBox are much too small. You will risk running out of space.
If creating a Ubuntu virtual machine under VirtualBox, do not partition the
disk using LVM, which is the Ubuntu installer default. Instead do all
partitioning with ext4 which is offered as an alternative to LVM. This way
if your vdi container does run out of space, you can use VirtualBox and gparted 
utilities to increase the container size without needing to resize 
LVM Volume Groups as an additional task. You may never have a need for LVM
in a virtual machine.  


## Getting the code

```bash
$ git clone https://github.com/atom/electron.git
```

## Bootstrapping

The bootstrap script will download all necessary build dependencies and create
build project files. You must have Python 2.7.x for the script to succeed. 
Downloading certain files could take a long time. Notice that we are using 
`ninja` to build Electron so there is no `Makefile` generated.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## Building

If you would like to build both `Release` and `Debug` targets:

```bash
$ ./script/build.py
```

This script will cause a very large Electron executable to be placed in
the directory `out/R`. The file size is in excess of 1.3 gigabytes. This 
happens because the Release target binary contains debugging symbols. 
To reduce the file size, run the `create-dist.py` script:

```bash
$ ./script/create-dist.py
```

This will put a working distribution with much smaller file sizes in 
the `dist` directory. After running the create-dist.py script, you 
may want to remove the 1.3+ gigabyte binary which is still in out/R.

You can also build the `Debug` target only:

```bash
$ ./script/build.py -c D
```

After building is done, you can find the `electron` debug binary 
under `out/D`.

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

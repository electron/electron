# Build Instructions (Linux)

遵循下面的引导，在 Linux 上构建 Electron .

## Prerequisites

* Python 2.7.x. 一些发行版如 CentOS 仍然使用 Python 2.6.x ，所以或许需要 check 你的 Python 版本，使用 `python -V`.
* Node.js v0.12.x. 有很多方法来安装 Node. 可以从 [Node.js](http://nodejs.org)下载原文件并且编译它 .也可以作为一个标准的用户在 home 目录下安装 node .或者尝试使用仓库 [NodeSource](https://nodesource.com/blog/nodejs-v012-iojs-and-the-nodesource-linux-repositories).
* Clang 3.4 或更新的版本.
* GTK+开发头文件和libnotify.

在 Ubuntu, 安装下面的库 :

```bash
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 libnss3-dev gcc-multilib g++-multilib
```

在 Fedora, 安装下面的库 :

```bash
$ sudo yum install clang dbus-devel gtk2-devel libnotify-devel libgnome-keyring-devel \
                   xorg-x11-server-utils libcap-devel cups-devel libXtst-devel \
                   alsa-lib-devel libXrandr-devel GConf2-devel nss-devel
```

其它版本的也许提供了相似的包来安装，通过包管理器，例如 pacman. 
或一个可以编译源文件的.

## 使用虚拟机  

如果在虚拟机上构建 Electron，你需要一个固定大小的设备，至少需要 25 gigabytes .

## 获取代码

```bash
$ git clone https://github.com/electron/electron.git
```

## Bootstrapping

bootstrap 脚本也是必要下载的构建依赖，来创建项目文件.需要使用 Python 2.7.x 来让脚本成功执行.正确下载文件会花费较长的时间.
注意我们使用的是 `ninja` 来构建 Electron，所以没有生成 `Makefile` 项目.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

### 交叉编译

如果想创建一个 `arm` target ，应当还要下载下面的依赖 :

```bash
$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \
                       g++-arm-linux-gnueabihf
```

为了编译 `arm` 或 `ia32` targets, 你应当为 `bootstrap.py` 脚本使用
`--target_arch` 参数:

```bash
$ ./script/bootstrap.py -v --target_arch=arm
```

## 构建

创建 `Release` 、 `Debug` target:

```bash
$ ./script/build.py
```

这个脚本也许会在目录 `out/R` 下创建一个巨大的可执行的 Electron . 文件大小或许会超过 1.3 gigabytes. 原因是 Release target 二进制文件包含了 调试符号 .运行 `create-dist.py` 脚本来减小文件的 size :

```bash
$ ./script/create-dist.py
```
这会在 `dist` 目录下创建一个有大量小文件的工作空间. 运行 create-dist.py 脚本之后, 或许你想删除仍然在 `out/R` 下的 1.3+ gigabyte 二进制文件.

可以只创建 `Debug` target:

```bash
$ ./script/build.py -c D
```

创建完毕, 可以在 `out/D`下面找到 `electron`.

## Cleaning

删除构建文件 :

```bash
$ ./script/clean.py
```

## 解决问题

确保你已经安装了所有的依赖 .

### Error While Loading Shared Libraries: libtinfo.so.5

预构建的 `clang` 会尝试链接到 `libtinfo.so.5`. 取决于  host 架构, 适当的使用 `libncurses`:

```bash
$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5
```

## Tests

测试你的修改是否符合项目代码风格，使用:

```bash
$ ./script/cpplint.py
```

测试有效性使用:

```bash
$ ./script/test.py
```
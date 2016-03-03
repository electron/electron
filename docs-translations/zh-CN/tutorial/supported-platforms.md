# 支持的平台

以下的平台是 Electron 目前支持的：

### OS X

对于 OS X 系统仅有64位的二进制文档，支持的最低版本是 OS X 10.8。

### Windows

仅支持 Windows 7 及其以后的版本，之前的版本中是不能工作的。

对于 Windows 提供 `x86` 和 `amd64` (x64) 版本的二进制文件。需要注意的是
`ARM` 版本的 Windows 目前尚不支持.

### Linux

预编译的 `ia32`(`i686`) 和 `x64`(`amd64`) 版本 Electron 二进制文件都是在
Ubuntu 12.04 下编译的，`arm` 版的二进制文件是在 ARM v7（硬浮点 ABI 与
Debian Wheezy 版本的 NEON）下完成的。

预编译二进制文件是否能够运行，取决于其中是否包括了编译平台链接的库，所以只有 Ubuntu 12.04
可以保证正常工作，但是以下的平台也被证实可以运行 Electron 的预编译版本：

* Ubuntu 12.04 及更新
* Fedora 21
* Debian 8

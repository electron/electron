# 支持的平台

目前 Electron 支持以下平台：

### macOS

对于 macOS 仅提供64位版本，并且只支持 macOS 10.9 或更高版本。

### Windows

仅支持 Windows 7 或更高版本。

对于 Windows 提供 `ia32` (x86) 和 `amd64` (x64) 版本。需要注意的是 `ARM` 版本的 Windows 目前尚不支持。

### Linux

Electron 的 `ia32` (`i686`) 和 `x64` (`amd64`) 预编译版本均是在Ubuntu 12.04 下编译的，`arm` 版的二进制文件是在 ARM v7（硬浮点 ABI 与
Debian Wheezy 版本的 NEON）下完成的。

预编译版本是否能够正常运行，取决于其中是否包含了编译平台的链接库。所以只有 Ubuntu 12.04 是可以保证能正常运行的，并且以下平台也被证实可以正常运行 Electron 的预编译版本：

* Ubuntu 12.04 或更高版本
* Fedora 21
* Debian 8

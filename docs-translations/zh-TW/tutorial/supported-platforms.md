# 支援的平台

Electron 已支援以下平台：

### macOS

macOS 系統只有 64 位元的執行檔，且 macOS 的最低版本要求為 macOS 10.8。

### Windows

Windows 7 和更新的版本都有支援，早期的作業系統都不支援(且無法運作)。



`x86` 和 `amd64` (x64) 的執行檔都有提供，請注意，`ARM` 版本的 Electron 還沒有支援。

### Linux

已經建置好的 `ia32`(`i686`) 和 `x64`(`amd64`) Electron 執行檔都是在 Ubuntu 12.04 的環境下編譯，`arm` 執行檔是在 hard-float ABI 和
Debian Wheezy 的 NEON 的 ARM v7 下編譯的。

已建置好的執行檔是否能夠成功在 Linux 發行版執行，要看該發行版在建置的平台上是否含有 Electron 會連結的函式庫，所以只有 Ubuntu 12.04 是已確定可以運行的，而以下平台也都有驗證過可以運行已建置好的 Electron 執行檔：

* Ubuntu 12.04 and later
* Fedora 21
* Debian 8

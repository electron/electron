# Windows商店指南

在 Windows 8 中, 一些不错的旧 win32 程序迎来了一个新朋友: 通用Windows平台(UWP)。 新的 `.appx` 格式不仅启用了许多新的强大的 API，如 Cortana 或推送通知，而且通过Windows 应用商店，也同时简化了安装和更新。

Microsoft 开发了一个工具，将 Electron 应用程序[编译为 `.appx` 软件包][electron-windows-store]，使开发人员能够使用新应用程序模型中的一些好东西。 本指南解释了如何使用它 - 以及 Electron AppX 包的功能和限制。

## 背景和要求

Windows 10 的 "周年更新" 能够运行 win32 `.exe` 程序并且它们的虚拟化文件系统和注册表跟随一起启动。 两者都是通过在 Windows 容器中运行应用程序和安装器编译后创建的，允许 Windows 在安装过程中正确识别操作系统进行了哪些修改。 将可执行文件和虚拟文件系统与虚拟注册表配对, 允许 Windows 启用一键安装和卸载。

此外，exe 在 appx 模型内启动 - 这意味着它可以使用通用 Windows 平台可用的许多 API。 为了获得更多的功能，Electron 应用程序可以与一个看不见的 UWP 后台任务配合使用，它与 `exe` 一起启动，作为后台运行任务的接收器，接收推送通知或与其他 UWP 应用程序通信 。

要编译任何现有的 Electron 应用程序，请确保满足以下要求:

* Windows 10及周年更新 (2016年8月2日发布的)
* Windows 10 SDK, [这里下载][windows-sdk]
* 最新的 Node 4 (运行 `node -v` 来确认)

然后, 安装 `electron-windows-store` CLI:

```
npm install -g electron-windows-store
```

## 步骤 1: 打包你的 Electron 应用程序

打包应用程序使用 [electron-packager][electron-packager] (或类似工具). 确保在最终的应用程序中删除不需要的 `node_modules`, 因为这些你不需要模块只会额外增加你的应用程序的大小.

结构输出应该看起来大致像这样:

```
├── Ghost.exe
├── LICENSE
├── content_resources_200_percent.pak
├── content_shell.pak
├── d3dcompiler_47.dll
├── ffmpeg.dll
├── icudtl.dat
├── libEGL.dll
├── libGLESv2.dll
├── locales
│   ├── am.pak
│   ├── ar.pak
│   ├── [...]
├── natives_blob.bin
├── node.dll
├── resources
│   ├── app
│   └── atom.asar
├── snapshot_blob.bin
├── squirrel.exe
├── ui_resources_200_percent.pak
└── xinput1_3.dll
```

## 步骤 2: 运行 electron-windows-store

从提权的 PowerShell（用管理员身份运行它）中,以所需的参数运行 `electron-windows-store`，传递输入和输出目录，应用程序的名称和版本，以及确认`node_modules`应该是扁平的。


```
electron-windows-store `
    --input-directory C:\myelectronapp `
    --output-directory C:\output\myelectronapp `
    --flatten true `
    --package-version 1.0.0.0 `
    --package-name myelectronapp
```

一旦执行，工具就开始工作：它接受您的 Electron 应用程序作为输入，展平 `node_modules`。 然后，它将应用程序归档为 `app.zip`。 使用安装程序和 Windows 容器，该工具创建一个“扩展的” AppX 包 - 包括 Windows 应用程序清单 （`AppXManifest.xml`）以及虚拟文件系统和输出文件夹中的虚拟注册表。

当创建扩展的 AppX 文件后，该工具使用 Windows App Packager（`MakeAppx.exe`）将磁盘上的这些文件创建为单文件 AppX 包。 最后，该工具可用于在计算机上创建可信证书，以签署新的 AppX 包。 使用签名的 AppX 软件包，CLI也可以自动在您的计算机上安装软件包。


## 步骤 3: 使用 AppX 包

为了运行您的软件包，您的用户将需要将 Windows 10 安装“周年纪念更新” - 有关如何更新Windows的详细信息可以在[这里][how-to-update]找到

与传统的UWP应用程序不同，打包应用程序目前需要进行手动验证过程，您可以在[这里][centennial-campaigns]申请.
在此期间，所有用户都能够通过双击安装包来安装您的程序，所以如果您只是寻找一个更简单的安装方法，可能不需要提交到商店。

在受管理的环境中（通常是企业）, `Add-AppxPackage` PowerShell Cmdlet 可用于以[自动方式安装][add-appxpackage]它。

另一个重要的限制是编译的 AppX 包仍然包含一个 win32 可执行文件，因此不会在 Xbox，HoloLens 或 Phones 中运行。

## 可选: 使用 BackgroundTask 添加 UWP 功能

您可以将 Electron 应用程序与不可见的 UWP 后台任务配对，以充分利用 Windows 10 功能，如推送通知，Cortana 集成或活动磁贴。

如何使用 Electron 应用程序通过后台任务发送 Toast 通知和活动磁贴,请查看[微软提供的案例][background-task].


## 可选: 使用容器虚拟化进行转换

要生成 AppX 包，`electron-windows-store` CLI 使用的模板应该适用于大多数 Electron 应用程序。 但是，如果您使用自定义安装程序，或者您遇到生成的包的任何问题，您可以尝试使用 Windows 容器编译创建包 - 在该模式下，CLI 将在空 Windows 容器中安装和运行应用程序，以确定应用程序正在对操作系统进行哪些修改。

在运行 CLI 之前，您必须设置 “Windows Desktop App Converter” 。 这将需要几分钟，但不要担心 - 你只需要这样做一次。 从这里下载 [Desktop App Converter][app-converter]

您将得到两个文件： `DesktopAppConverter.zip` 和 `BaseImage-14316.wim`.

1. 解压 `DesktopAppConverter.zip`. 打开提权的 PowerShell (用"以管理员权限运行"打开, 确保您的系统执行策略允许我们通过调用 `Set-ExecutionPolicy bypass` 来运行我们想要运行的一切).
2. 然后, 通过调用 `.\DesktopAppConverter.ps1 -Setup -BaseImage .\BaseImage-14316.wim`, 运行 Desktop App Converter 安装，并传递 Windows 基本映像的位置 (下载的 `BaseImage-14316.wim`).
3. 如果运行以上命令提示您重新启动，请重新启动计算机，并在成功重新启动后再次运行上述命令。

当安装成功后，您可以继续编译你的 Electron 应用程序。

[windows-sdk]: https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk
[app-converter]: https://www.microsoft.com/en-us/download/details.aspx?id=51691
[add-appxpackage]: https://technet.microsoft.com/en-us/library/hh856048.aspx
[electron-packager]: https://github.com/electron-userland/electron-packager
[electron-windows-store]: https://github.com/catalystcode/electron-windows-store
[background-task]: https://github.com/felixrieseberg/electron-uwp-background
[centennial-campaigns]: https://developer.microsoft.com/en-us/windows/projects/campaigns/desktop-bridge
[how-to-update]: https://blogs.windows.com/windowsexperience/2016/08/02/how-to-get-the-windows-10-anniversary-update

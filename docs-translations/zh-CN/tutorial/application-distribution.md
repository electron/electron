# 应用部署

为了使用Electron部署你的应用程序，你存放应用程序的文件夹需要叫做 `app` 并且需要放在 Electron 的资源文件夹下（在 OS X 中是指 `Electron.app/Contents/Resources/`，在 Linux 和 Windows 中是指 `resources/`）
就像这样：

在 OS X 中:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

在 Windows 和 Linux 中:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

然后运行 `Electron.app` （或者 Linux 中的 `electron`，Windows 中的 `electron.exe`）,
接着 Electron 就会以你的应用程序的方式启动。`electron` 文件夹将被部署并可以分发给最终的使用者。

## 将你的应用程序打包成一个文件

除了通过拷贝所有的资源文件来分发你的应用程序之外，你可以可以通过打包你的应用程序为一个 [asar](https://github.com/atom/asar) 库文件以避免暴露你的源代码。

为了使用一个 `asar` 库文件代替 `app` 文件夹，你需要修改这个库文件的名字为 `app.asar` ，
然后将其放到 Electron 的资源文件夹下，然后 Electron 就会试图读取这个库文件并从中启动。
如下所示：

在 OS X 中:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

在 Windows 和 Linux 中:

```text
electron/resources/
└── app.asar
```

更多的细节请见 [Application packaging](application-packaging.md).

## 更换名称与下载二进制文件

在使用 Electron 打包你的应用程序之后，你可能需要在分发给用户之前修改打包的名字。

### Windows

你可以将 `electron.exe` 改成任意你喜欢的名字，然后可以使用像
[rcedit](https://github.com/atom/rcedit) 或者[ResEdit](http://www.resedit.net)
编辑它的icon和其他信息。

### OS X

你可以将 `Electron.app` 改成任意你喜欢的名字，然后你也需要修改这些文件中的 
`CFBundleDisplayName`， `CFBundleIdentifier` 以及 `CFBundleName` 字段。
这些文件如下：

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

你也可以重命名帮助应用程序以避免在应用程序监视器中显示 `Electron Helper`，
但是请确保你已经修改了帮助应用的可执行文件的名字。

一个改过名字的应用程序的构造可能是这样的：

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

你可以将 `electron` 改成任意你喜欢的名字。

## 通过重编译源代码来更换名称

通过修改产品名称并重编译源代码来更换 Electron 的名称也是可行的。
你需要修改 `atom.gyp` 文件并彻底重编译一次。

### grunt打包脚本

手动的检查 Electron 代码并重编译是很复杂晦涩的，因此有一个 Grunt任务可以自动自动的处理
这些内容 [grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

这个任务会自动的处理编辑 `.gyp` 文件，从源代码进行编译，然后重编译你的应用程序的本地 Node 模块以匹配这个新的可执行文件的名称。

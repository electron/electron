# 使用原生模块

Electron 同样也支持原生模块，但由于和官方的 Node 相比使用了不同的 V8 引擎，如果你想编译原生模块，则需要手动设置 Electron 的 headers 的位置。

## 原生Node模块的兼容性

当 Node 开始换新的V8引擎版本时，原生模块可能“坏”掉。为确保一切工作正常，你需要检查你想要使用的原生模块是否被 Electron 内置的 Node 支持。你可以在[这里](https://github.com/electron/electron/releases)查看 Electron 内置的 Node 版本，或者使用 `process.version` (参考：[快速入门](https://github.com/electron/electron/blob/master/docs/tutorial/quick-start.md))查看。

考虑到 [NAN](https://github.com/nodejs/nan/) 可以使你的开发更容易对多版本 Node 的支持，建议使用它来开发你自己的模块。你也可以使用 [NAN](https://github.com/nodejs/nan/) 来移植旧的模块到新的 Nod e版本，以使它们可以在新的 Electron 下良好工作。

## 如何安装原生模块

如下三种方法教你安装原生模块：

### 最简单方式

最简单的方式就是通过 [`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild) 包重新编译原生模块，它帮你自动完成了下载 headers、编译原生模块等步骤：

```sh
npm install --save-dev electron-rebuild

# 每次运行"npm install"时，也运行这条命令
./node_modules/.bin/electron-rebuild

# 在windows下如果上述命令遇到了问题，尝试这个：
.\node_modules\.bin\electron-rebuild.cmd
```

### 通过 npm 安装

你当然也可以通过 `npm` 安装原生模块。大部分步骤和安装普通模块时一样，除了以下一些系统环境变量你需要自己操作：

```bash
export npm_config_disturl=https://atom.io/download/electron
export npm_config_target=0.33.1
export npm_config_arch=x64
export npm_config_runtime=electron
HOME=~/.electron-gyp npm install module-name
```

### 通过 node-gyp 安装

你需要告诉 `node-gyp` 去哪下载 Electron 的 headers，以及下载什么版本：

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/electron
```

`HOME=~/.electron-gyp` 设置去哪找开发时的 headers。

`--target=0.29.1` 设置了 Electron 的版本

`--dist-url=...` 设置了 Electron 的 headers 的下载地址

`--arch=x64` 设置了该模块为适配64位操作系统而编译

# 使用原生模块

Electron 同样也支持原生模块，但由于和官方的 Node 相比使用了不同的 V8 引擎，如果你想编译原生模块，则需要手动设置 Electron 的 headers 的位置。

## 如何安装原生模块

如下三种方法教你安装原生模块：

### 通过 npm 安装

只要设置一些系统环境变量，你就可以通过 `npm` 直接安装原生模块。

为 Electron 安装所有依赖项的一个例子：

```bash
# Electron 的版本。
export npm_config_target=1.2.3
# Electron 的系统架构, 值为 ia32 或者 x64。
export npm_config_arch=x64
export npm_config_target_arch=x64
# 下载 Electron 的 headers。
export npm_config_disturl=https://atom.io/download/electron
# 告诉 node-pre-gyp 是为 Electron 构建。
export npm_config_runtime=electron
# 告诉 node-pre-gyp 从源代码构建模块。
export npm_config_build_from_source=true
# 下载所有依赖，并缓存到 ~/.electron-gyp。
HOME=~/.electron-gyp npm install
```
### 下载并重新编译模块

你可以选择安装其他 Node 项目的模块，通过 [`electron-rebuild`][electron-rebuild] 包
重新编译原生模块，它可以识别当前 Electron 版本，帮你自动完成了下载 headers、编译原生模块等步骤。

一个下载 `electron-rebuild` 并重新编译的例子：

```bash
npm install --save-dev electron-rebuild

# 每次运行"npm install"时，也运行这条命令
./node_modules/.bin/electron-rebuild

# 在windows下如果上述命令遇到了问题，尝试这个：
.\node_modules\.bin\electron-rebuild.cmd
```

### 手动编译

如果你是一个原生模块的开发人员，想在 Electron 中进行测试，
你可能要手动编译 Electron 模块。你可以
使用 `node-gyp` 直接编译：

```bash
cd /path-to-module/
HOME=~/.electron-gyp node-gyp rebuild --target=1.2.3 --arch=x64 --dist-url=https://atom.io/download/electron
```
`HOME=~/.electron-gyp` 设置去哪找开发时的 headers。
`--target=0.29.1` 设置了 Electron 的版本。
`--dist-url=...` 设置了 Electron 的 headers 的下载地址。
`--arch=x64` 设置了该模块为适配64位操作系统而编译。

## 故障排除

如果你安装了一个原生模块并发现它不能工作，你需要检查
以下事项：

* 模块的对应的操作系统和 Electron 对应的操作系统是否匹配(ia32 或 x64)。
* 如果升级了 Electron，你通常需要重新编译这些模块。
* 当有疑问时，请先执行 `electron-rebuild`。

## `prebuild` 依赖模块

[`prebuild`](https://github.com/mafintosh/prebuild) 为多个版本的 Node 和 Electron 提供了一种简单发布预编译二进制原生模块的方法。

如果为 Electron 提供二进制原生模块，请确保删除
`--build-from-source` 和 `npm_config_build_from_source` 环境变量
来充分利用预编译的二进制文件。

## `node-pre-gyp` 依赖模块

[`node-pre-gyp` 工具][node-pre-gyp] 提供一种部署原生 Node 预编译二进制模块的方法，
许多流行的模块都是使用它。

通常这些模块在 Electron 中工作良好，但有时当 Electron 使用
比 Node 新的 V8 版本时，会有 ABI 改变，可能
发生错误。因此，一般来说，建议始终从源代码编译原生模块。

如果你通过 `npm` 的方式安装模块，默认情况下这就完成了，
如果没有，你需要传入 `--build-from-source` 给 `npm`, 或者设置
`npm_config_build_from_source` 环境变量。

[electron-rebuild]: https://github.com/paulcbetts/electron-rebuild
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp

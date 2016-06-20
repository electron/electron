# Build Instructions (macOS)

遵循下面的引导，在 macOS 上构建 Electron .

## 前提

* macOS >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (外部)

如果你通过 Homebrew 使用 Python 下载，需要安装下面的 Python 模块:

* pyobjc

## 获取代码

```bash
$ git clone https://github.com/electron/electron.git
```

## Bootstrapping

bootstrap 脚本也是必要下载的构建依赖，来创建项目文件.注意我们使用的是 [ninja](https://ninja-build.org/) 来构建 Electron，所以没有生成 Xcode 项目.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## 构建

创建 `Release` 、 `Debug` target:

```bash
$ ./script/build.py
```

可以只创建 `Debug` target:

```bash
$ ./script/build.py -c D
```

创建完毕, 可以在 `out/D` 下面找到 `Electron.app`.

## 32位支持

在 macOS 上，构建 Electron 只支持 64位的，不支持 32位的 .

## 测试

测试你的修改是否符合项目代码风格，使用:

```bash
$ ./script/cpplint.py
```

测试有效性使用:

```bash
$ ./script/test.py
```
# Build Instructions (Windows)

遵循下面的引导，在 Windows 上构建 Electron .

## 前提

* Windows 7 / Server 2008 R2 or higher
* Visual Studio 2013 with Update 4 - [download VS 2013 Community Edition for
  free](https://www.visualstudio.com/news/vs2013-community-vs).
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [Node.js](http://nodejs.org/download/)
* [Git](http://git-scm.com)

如果你现在还没有安装 Windows , [modern.ie](https://www.modern.ie/en-us/virtualization-tools#downloads) 有一个 timebombed 版本的 Windows ，你可以用它来构建 Electron.

构建 Electron 完全的依赖于命令行，并且不可通过 Visual Studio.
可以使用任何的编辑器来开发 Electron ，未来会支持 Visual Studio.

**注意:** 虽然 Visual Studio 不是用来构建的，但是它仍然
**必须的** ，因为我们需要它提供的构建工具栏.

**注意:** Visual Studio 2015 不可用. 请确定使用 MSVS
**2013**.

## 获取代码

```powershell
$ git clone https://github.com/electron/electron.git
```

## Bootstrapping

bootstrap 脚本也是必要下载的构建依赖，来创建项目文件.注意我们使用的是 `ninja` 来构建 Electron，所以没有生成 Visual Studio 项目.

```powershell
$ cd electron
$ python script\bootstrap.py -v
```

## 构建

创建 `Release` 、 `Debug` target:

```powershell
$ python script\build.py
```

可以只创建 `Debug` target:

```powershell
$ python script\build.py -c D
```

创建完毕, 可以在 `out/D`(debug target) 或 `out\R` (release target) 下面找到 `electron.exe`.

## 64bit Build

为了构建64位的 target,在运行 bootstrap 脚本的时候需要使用 `--target_arch=x64` :

```powershell
$ python script\bootstrap.py -v --target_arch=x64
```

其他构建步骤完全相同.

## Tests

测试你的修改是否符合项目代码风格，使用:

```powershell
$ python script\cpplint.py
```

测试有效性使用:

```powershell
$ python script\test.py
```
在构建 debug 时为 Tests包含原生模块 (例如 `runas`) 将不会执行(详情 [#2558](https://github.com/electron/electron/issues/2558)), 但是它们在构建 release 会起效.

运行 release 构建使用 :

```powershell
$ python script\test.py -R
```

## 解决问题

### Command xxxx not found

如果你遇到了一个错误，类似 `Command xxxx not found`, 可以尝试使用 `VS2012 Command Prompt` 控制台来执行构建脚本 .

### Fatal internal compiler error: C1001

确保你已经安装了 Visual Studio 的最新安装包 .

### Assertion failed: ((handle))->activecnt >= 0

如果在 Cygwin 下构建的，你可能会看到 `bootstrap.py` 失败并且附带下面错误 :

```
Assertion failed: ((handle))->activecnt >= 0, file src\win\pipe.c, line 1430

Traceback (most recent call last):
  File "script/bootstrap.py", line 87, in <module>
    sys.exit(main())
  File "script/bootstrap.py", line 22, in main
    update_node_modules('.')
  File "script/bootstrap.py", line 56, in update_node_modules
    execute([NPM, 'install'])
  File "/home/zcbenz/codes/raven/script/lib/util.py", line 118, in execute
    raise e
subprocess.CalledProcessError: Command '['npm.cmd', 'install']' returned non-zero exit status 3
```

这是由同时使用 Cygwin Python 和 Win32 Node 造成的 bug.解决办法就是使用 Win32 Python 执行 bootstrap 脚本 (假定你已经在目录 `C:\Python27` 下安装了 Python):

```powershell
$ /cygdrive/c/Python27/python.exe script/bootstrap.py
```

### LNK1181: cannot open input file 'kernel32.lib'

重新安装 32位的 Node.js.

### Error: ENOENT, stat 'C:\Users\USERNAME\AppData\Roaming\npm'

简单创建目录 [应该可以解决问题](http://stackoverflow.com/a/25095327/102704):

```powershell
$ mkdir ~\AppData\Roaming\npm
```

### node-gyp is not recognized as an internal or external command

如果你使用 Git Bash 来构建，或许会遇到这个错误，可以使用 PowerShell 或 VS2012 Command Prompt 来代替 .
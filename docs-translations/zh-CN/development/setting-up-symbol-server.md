# Setting Up Symbol Server in Debugger

调试 symbols 让你有更好的调试 sessions. 它们有可执行的动态库的函数信息，并且提供信息来获得洁净的呼叫栈. 一个 Symbol 服务器允许调试器自动加载正确的 symbols, 二进制文件 和 资源文件，不用再去强制用户下载巨大的调试文件. 服务器函数类似
[Microsoft's symbol server](http://support.microsoft.com/kb/311503) ，所以这里的记录可用.

注意，因为公众版本的 Electron 构建是最优化的，调试不一定一直简单.调试器将不会给显示出所有变量内容，并且因为内联，尾调用，和其它编译器优化，执行路径会看起来很怪异 . 唯一的解决办法是搭建一个不优化的本地构建.

Electron 使用的官方 symbol 服务器地址为 
`http://54.249.141.255:8086/atom-shell/symbols` .
你不能直接访问这个路径，必须将其添加到你的调试工具的 symbol 路径上.在下面的例子中，使用了一个本地缓存目录来避免重复从服务器获取 PDB.  在你的电脑上使用一个恰当的缓存目录来代替 `c:\code\symbols` .

## Using the Symbol Server in Windbg

Windbg symbol 路径被配制为一个限制带星号字符的字符串. 要只使用  Electron 的 symbol 服务器, 将下列记录添加到你的 symbol 路径 (__注意:__ 如果你愿意使用一个不同的地点来下载 symbols，你可以在你的电脑中使用任何可写的目录来代替 `c:\code\symbols`):

```
SRV*c:\code\symbols\*http://54.249.141.255:8086/atom-shell/symbols
```

使用 Windbg 菜单或通过输入 `.sympath` 命令，在环境中设置一个 `_NT_SYMBOL_PATH` 字符串.如果你也想从微软的 symbol 服务器获得 symbols ，你应当首先将它们先列出来 :

```
SRV*c:\code\symbols\*http://msdl.microsoft.com/download/symbols;SRV*c:\code\symbols\*http://54.249.141.255:8086/atom-shell/symbols
```

## 在 Visual Studio 中使用 symbol 服务器

<img src='http://mdn.mozillademos.org/files/733/symbol-server-vc8express-menu.jpg'>
<img src='http://mdn.mozillademos.org/files/2497/2005_options.gif'>

## Troubleshooting: Symbols will not load

在 Windbg 中输入下列命令，打印出为什么 symbols 没有加载 :

```
> !sym noisy
> .reload /f chromiumcontent.dll
```

# 编码规范

以下是 Electron 项目中代码书写规范的指导方针。

## C++ 和 Python

对于 C++ 和 Python，我们追随 Chromium 的[Coding
Style](http://www.chromium.org/developers/coding-style)。你可以通过 `script/cpplint.py` 来检验所有文件是否符合要求。

我们使用的 Pyhton 版本是 Python 2.7。

其中 C++ 代码中用到了许多 Chromium 的抽象和类型，我们希望你对其有所熟悉。一个好的去处是
Chromium 的[重要的抽象和数据结构](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures)。这个文档提到了一些特殊的类型、域内类型（当超出作用域时会自动释放内存）、日志机制等等。

## CoffeeScript

对于 CoffeeScript，我们追随 GitHub 的[Style
Guide](https://github.com/styleguide/javascript) 及如下规则:

* 文件不应该以换行结尾，因为我们要匹配 Google 的规范。
* 文件名应该以 `-` 作连接而不是 `_`，等等。
  `file-name.coffee` 而不是 `file_name.coffee`，因为在
  [github/atom](https://github.com/github/atom) 模块名通常都是 `module-name` 的形式。这条规则仅应用于 `.coffee` 文件。

## API 名称

当新建一个API时，我们应该倾向于 getters 和 setters 的方式，而不是
jQuery 的单函数形式。例如，`.getText()` 和 `.setText(text)`
 优于 `.text([text])`。这里是相关的
[讨论记录](https://github.com/atom/electron/issues/46)。

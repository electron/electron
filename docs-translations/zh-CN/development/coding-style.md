# 编码规范

以下是 Electron 项目的编码规范。

## C++ 和 Python

对于 C++ 和 Python，我们遵循 Chromium 的[编码规范](http://www.chromium.org/developers/coding-style)。你可以使用 `script/cpplint.py` 来检验文件是否符合要求。

我们目前使用的 Pyhton 版本是 Python 2.7。

C++ 代码中用到了许多 Chromium 中的接口和数据类型，所以希望你能熟悉它们。Chromium  中的[重要接口和数据结构](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures)就是一篇不错的入门文档，里面提到了一些特殊类型、域内类型（退出作用域时自动释放内存）、日志机制，等等。

## CoffeeScript

对于 CoffeeScript，我们遵循 GitHub 的[编码规范](https://github.com/styleguide/javascript) 及以下规则:

* 文件**不要**以换行符结尾，我们要遵循 Google 的编码规范。
* 文件名使用 `-` 而不是 `_` 来连接单词，比如 `file-name.coffee` 而不是 `file_name.coffee`，这是沿用 [github/atom](https://github.com/github/atom) 模块的命名方式（`module-name`）。这条规则仅适用于 `.coffee` 文件。

## API 命名

当新建一个 API 时，我们倾向于使用 getters 和 setters 而不是 jQuery 单函数的命名方式，比如 `.getText()` 和 `.setText(text)`
 而不是 `.text([text])`。这里有关于该规则的[讨论记录](https://github.com/electron/electron/issues/46)。

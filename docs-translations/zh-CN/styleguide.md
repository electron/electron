# Electron 文档风格指南

这里是一些编写 Electron 文档的指南.

## 标题

* 每个页面顶部必须有一个单独的 `＃` 级标题。
* 同一页面中的章节必须有 `##` 级标题。
* 子章节需要根据它们的嵌套深度增加标题中的 `＃` 数量。
* 页面标题中的所有单词首字母都必须大写，除了 “of” 和 “and” 之类的连接词。
* 只有章节标题的第一个单词首字母必须大写.

举一个 `Quick Start` 的例子:

```markdown
# Quick Start

...

## Main process

...

## Renderer process

...

## Run your app

...

### Run as a distribution

...

### Manually downloaded Electron binary

...
```

对于 API 参考, 可以例外于这些规则.

## Markdown 规则

* 在代码块中使用 `bash` 而不是 `cmd`（由于语法高亮问题）.
* 行长度应该控制在80列内.
* 列表嵌套不超出2级 (由于 Markdown 渲染问题).
* 所有的 `js` 和 `javascript` 代码块均被标记为
[standard-markdown](http://npm.im/standard-markdown).

## 用词选择

* 在描述结果时，使用 “will” 而不是 “would”。
* 首选 "in the ___ process" 而不是 "on".

## API 参考

以下规则仅适用于 API 的文档。

### 页面标题

每个页面必须使用由 `require（'electron'）` 返回的实际对象名称作为标题，例如 `BrowserWindow`，`autoUpdater` 和 `session`。

在页面标题下必须是以 `>` 开头的单行描述。

举一个 `session` 的例子:

```markdown
# session

> Manage browser sessions, cookies, cache, proxy settings, etc.
```

### 模块方法和事件

对于非类的模块，它们的方法和事件必须在 `## Methods` 和 `## Events` 章节中列出。

举一个 `autoUpdater` 的例子:

```markdown
# autoUpdater

## Events

### Event: 'error'

## Methods

### `autoUpdater.setFeedURL(url[, requestHeaders])`
```

### 类

* API 类或作为模块一部分的类必须在 `## Class: TheClassName` 章节中列出.
* 一个页面可以有多个类.
* 构造函数必须用 `###` 级标题列出.
* [静态方法](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes/static) 必须在 `### Static Methods` 章节中列出.
* [实例方法](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Classes#Prototype_methods) 必须在 `### Instance Methods` 章节中列出.
* 所有具有返回值的方法必须用 "Returns `[TYPE]` - Return description" 的形式描述.
  * 如果该方法返回一个 `Object`，则可以使用冒号后跟换行符，然后使用与函数参数相同样式的属性的无序列表来指定其结构.
* 实例事件必须在 `### Instance Events` 章节中列出.
* 实例属性必须在 `### Instance Properties` 章节中列出.
  * 实例属性必须以 "A [Property Type] ..." 开始描述.

这里用 `Session` 和 `Cookies` 类作为例子:

```markdown
# session

## Methods

### session.fromPartition(partition)

## Properties

### session.defaultSession

## Class: Session

### Instance Events

#### Event: 'will-download'

### Instance Methods

#### `ses.getCacheSize(callback)`

### Instance Properties

#### `ses.cookies`

## Class: Cookies

### Instance Methods

#### `cookies.get(filter, callback)`
```

### 方法

方法章节必须采用以下形式：

```markdown
### `objectName.methodName(required[, optional]))`

* `required` String - A parameter description.
* `optional` Integer (optional) - Another parameter description.

...
```

标题可以是 `###` 级别或 `####` 级别，具体取决于它是模块还是类的方法。

对于模块，`objectName` 是模块的名称。 对于类，它必须是类的实例的名称，并且不能与模块的名称相同。

例如，`session` 模块下的 `Session` 类的方法必须使用 `ses` 作为 `objectName` 。

可选参数由围绕可选参数的方括号 `[]` 表示，并且如果此可选参数跟随另一个参数，则需要逗号：

```
required[, optional]
```

下面的方法是每个参数更加详细的信息。 参数的类型由常见类型表示:

* [`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String)
* [`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number)
* [`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object)
* [`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
* [`Boolean`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Boolean)
* 或自定义类型,就像 Electron 的 [`WebContent`](api/web-contents.md)

如果参数或方法对某些平台是唯一的，那么这些平台将使用数据类型后面的空格分隔的斜体列表来表示。 值可以是 `macOS`，`Windows` 或 `Linux`

```markdown
* `animate` Boolean (optional) _macOS_ _Windows_ - Animate the thing.
```

`Array` 类型的参数, 必须在指定数组下面的描述中描述可能包含的元素.

`Function` 类型参数的描述应该清楚描述它是如何被调用的，并列出将被传递给它的参数的类型.

### 事件

事件章节必须采用以下形式:

```markdown
### Event: 'wake-up'

Returns:

* `time` String

...
```

标题可以是 `###` 级别或 `####` 级别，具体取决于它是模块还是类的事件。

事件的参数遵循与方法相同的规则.

### 属性

属性章节必须采用以下形式:

```markdown
### session.defaultSession

...
```

标题可以是 `###` 级别或 `####` 级别，具体取决于它是模块还是类的属性。

## 文档翻译

Electron 文档的翻译文件位于 `docs-translations` 目录中.

如要添加另一个设定集（或部分设定集）:

* 创建以语言缩写命名的子目录。
* 翻译文件。
* 更新您的语言目录中的 `README.md` 文件以链接到已翻译的文件。
* 在 Electron 的主 [README](https://github.com/electron/electron#documentation-translations) 上添加到翻译目录的链接。

请注意，`docs-translations` 下的文件只能包含已被翻译的文件，不应将原始英语文件复制到那里。

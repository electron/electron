# 应用打包

为舒缓Windows下路径名过长的问题[issues](https://github.com/joyent/node/issues/6960), 也略对`require`加速以及简单隐匿你的源代码， 你可以通过极小的源代码改动将你的应用打包成[asar][asar].

## 生成`asar`包

[asar][asar]是一种将多个文件合并成一个文件的类tar风格的归档格式。 Electron可以无需解压，即从其中读取任意文件内容。

参照如下步骤将你的应用打包成`asar`：

### 1. 安装asar

```bash
$ npm install -g asar
```

### 2. 用`asar pack`打包

```bash
$ asar pack your-app app.asar
```

## 使用`asar`包

在Electron中有两类APIs：Node.js提供的Node APIs和Chromium提供的Web APIs。这两种APIs都支持从`asar`包中读取文件。

### Node API

由于Electron中打了特别补丁， Node APIs中如`fs.readFile`或者`require`之类的方法可以将`asar`视之为虚拟文件夹，读取`asar`里面的文件就和从真实的文件系统中读取一样。

例如，假设我们在`/path/to`文件夹下有个`example.asar`包：

```bash
$ asar list /path/to/example.asar
/app.js
/file.txt
/dir/module.js
/static/index.html
/static/main.css
/static/jquery.min.js
```

从`asar`包读取一个文件：

```javascript
const fs = require('fs');
fs.readFileSync('/path/to/example.asar/file.txt');
```

列出`asar`包中根目录下的所有文件：

```javascript
const fs = require('fs');
fs.readdirSync('/path/to/example.asar');
```

使用`asar`包中的一个模块：

```javascript
require('/path/to/example.asar/dir/module.js');
```

你也可以使用`BrowserWindow`来显示一个`asar`包里的web页面：

```javascript
const BrowserWindow = require('electron').BrowserWindow;
var win = new BrowserWindow({width: 800, height: 600});
win.loadURL('file:///path/to/example.asar/static/index.html');
```

### Web API

在Web页面里，用`file:`协议可以获取`asar`包中文件。和Node API一样，视`asar`包如虚拟文件夹。

例如，用`$.get`获取文件:

```html
<script>
var $ = require('./jquery.min.js');
$.get('file:///path/to/example.asar/file.txt', function(data) {
  console.log(data);
});
</script>
```

### 像“文件”那样处理`asar`包

有些场景，如：核查`asar`包的校验和，我们需要像读取“文件”那样读取`asar`包的内容(而不是当成虚拟文件夹)。你可以使用内置的`original-fs`(提供和`fs`一样的APIs)模块来读取`asar`包的真实信息。

```javascript
var originalFs = require('original-fs');
originalFs.readFileSync('/path/to/example.asar');
```

## Node API缺陷

尽管我们已经尽了最大努力使得`asar`包在Node API下的应用尽可能的趋向于真实的目录结构，但仍有一些底层Node API我们无法保证其正常工作。

### `asar`包是只读的

`asar`包中的内容不可更改，所以Node APIs里那些可以用来修改文件的方法在对待`asar`包时都无法正常工作。

### Working Directory在`asar`包中无效

尽管`asar`包是虚拟文件夹，但其实并没有真实的目录架构对应在文件系统里，所以你不可能将working Directory设置成`asar`包里的一个文件夹。将`asar`中的文件夹以`cwd`形式作为参数传入一些API中也会报错。

### API中的额外“开箱”

大部分`fs`API可以无需解压即从`asar`包中读取文件或者文件的信息，但是在处理一些依赖真实文件路径的底层系统方法时，Electron会将所需文件解压到临时目录下，然后将临时目录下的真实文件路径传给底层系统方法使其正常工作。 对于这类API，耗费会略多一些。

以下是一些需要额外解压的APIs：

* `child_process.execFile`
* `child_process.execFileSync`
* `fs.open`
* `fs.openSync`
* `process.dlopen` - `require`native模块时用到

### `fs.stat`获取的stat信息不可靠

对`asar`包中的文件取`fs.stat`，返回的`Stats`对象不是精确值，因为这些文件不是真实存在于文件系统里。所以除了文件大小和文件类型以外，你不应该依赖`Stats`对象的值。

### 执行`asar`包中的程序

Node中有一些可以执行程序的API，如`child_process.exec`，`child_process.spawn`和`child_process.execFile`等，但只有`execFile`可以执行`asar`包中的程序。

因为`exec`和`spawn`允许`command`替代`file`作为输入，而`command`是需要在shell下执行的，目前没有可靠的方法来判断`command`中是否在操作一个`asar`包中的文件，而且即便可以判断，我们依旧无法保证可以在无任何副作用的情况下替换`command`中的文件路径。

## 打包时排除文件

如上所述，一些Node API会在调用时将文件解压到文件系统中，除了效率问题外，也有可能引起杀毒软件的注意！

为解决这个问题，你可以在生成`asar`包时使用`--unpack`选项来排除一些文件，使其不打包到`asar`包中，下面是如何排除一些用作共享用途的native模块的方法：

```bash
$ asar pack app app.asar --unpack *.node
```

经过上述命令后，除了生成的`app.asar`包以外，还有一个包含了排除文件的`app.asar.unpacked`文件夹，你需要将这个文件夹一起拷贝，提供给用户。

[asar]: https://github.com/atom/asar

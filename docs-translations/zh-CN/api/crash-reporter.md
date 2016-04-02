# crashReporter

`crash-reporter` 模块开启发送应用崩溃报告.

下面是一个自动提交崩溃报告给服务器的例子 :

```javascript
const crashReporter = require('electron').crashReporter;

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https://your-domain.com/url-to-submit',
  autoSubmit: true
});
```

可以使用下面的项目来创建一个服务器，用来接收和处理崩溃报告 :

* [socorro](https://github.com/mozilla/socorro)
* [mini-breakpad-server](https://github.com/atom/mini-breakpad-server)

## 方法

`crash-reporter` 模块有如下方法:

### `crashReporter.start(options)`

* `options` Object
  * `companyName` String
  * `submitURL` String - 崩溃报告发送的路径，以post方式.
  * `productName` String (可选) - 默认为 `Electron`.
  * `autoSubmit` Boolean - 是否自动提交.
    默认为 `true`.
  * `ignoreSystemCrashHandler` Boolean - 默认为 `false`.
  * `extra` Object - 一个你可以定义的对象，附带在崩溃报告上一起发送 . 只有字符串属性可以被正确发送，不支持嵌套对象.

只可以在使用其它 `crashReporter` APIs 之前使用这个方法.

**注意:** 在 OS X, Electron 使用一个新的 `crashpad` 客户端, 与 Windows 和 Linux 的 `breakpad` 不同. 为了开启崩溃点搜集，你需要在主进程和其它每个你需要搜集崩溃报告的渲染进程中调用  `crashReporter.start` API 来初始化 `crashpad`.

### `crashReporter.getLastCrashReport()`

返回最后一个崩溃报告的日期和 ID.如果没有过崩溃报告发送过来，或者还没有开始崩溃报告搜集，将返回 `null` .

### `crashReporter.getUploadedReports()`

返回所有上载的崩溃报告，每个报告包含了上载日期和 ID.

## crash-reporter Payload

崩溃报告将发送下面 `multipart/form-data` `POST` 型的数据给 `submitURL` :

* `ver` String - Electron 版本.
* `platform` String - 例如 'win32'.
* `process_type` String - 例如 'renderer'.
* `guid` String - 例如 '5e1286fc-da97-479e-918b-6bfb0c3d1c72'
* `_version` String - `package.json` 版本.
* `_productName` String - `crashReporter` `options`
  对象中的产品名字.
* `prod` String - 基础产品名字. 这种情况为 Electron.
* `_companyName` String - `crashReporter` `options`
  对象中的公司名字.
* `upload_file_minidump` File - 崩溃报告按照 `minidump` 的格式.
* `crashReporter` 中的 `extra` 对象的所有等级和一个属性.
  `options` object
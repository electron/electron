# contentTracing

`content-tracing` 模块是用来收集由底层的Chromium content 模块 产生的搜索数据. 这个模块不具备web接口，所有需要我们在chrome浏览器中添加 `chrome://tracing/` 来加载生成文件从而查看结果.

```javascript
const contentTracing = require('electron').contentTracing;

const options = {
  categoryFilter: '*',
  traceOptions: 'record-until-full,enable-sampling'
}

contentTracing.startRecording(options, function() {
  console.log('Tracing started');

  setTimeout(function() {
    contentTracing.stopRecording('', function(path) {
      console.log('Tracing data recorded to ' + path);
    });
  }, 5000);
});
```

## 方法

 `content-tracing` 模块的方法如下:

### `contentTracing.getCategories(callback)`

* `callback` Function

获得一组分类组. 分类组可以更改为新的代码路径。

一旦所有的子进程都接受到了`getCategories`方法请求, 分类组将调用 `callback`.

### `contentTracing.startRecording(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

开始向所有进程进行记录.(recording)

一旦收到可以开始记录的请求，记录将会立马启动并且在子进程是异步记录听的. 当所有的子进程都收到 `startRecording` 请求的时候，`callback` 将会被调用.

`categoryFilter`是一个过滤器，它用来控制那些分类组应该被用来查找.过滤器应当有一个可选的 `-` 前缀来排除匹配的分类组.不允许同一个列表既是包含又是排斥.

例子:

* `test_MyTest*`,
* `test_MyTest*,test_OtherStuff`,
* `"-excluded_category1,-excluded_category2`

`traceOptions` 控制着哪种查找应该被启动，这是一个用逗号分隔的列表.可用参数如下:

* `record-until-full`
* `record-continuously`
* `trace-to-console`
* `enable-sampling`
* `enable-systrace`

前3个参数是来查找记录模块，并且以后都互斥.如果在`traceOptions` 中超过一个跟踪
记录模式，那最后一个的优先级最高.如果没有指明跟踪
记录模式，那么它默认为 `record-until-full`.

在 `traceOptions` 中的参数被解析应用之前，查找参数初始化默认为 (`record_mode` 设置为
`record-until-full`, `enable_sampling` 和 `enable_systrace` 设置为 `false`).

### `contentTracing.stopRecording(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

停止对所有子进程的记录.

子进程通常缓存查找数据，并且仅仅将数据截取和发送给主进程.这有利于在通过 IPC 发送查找数据之前减小查找时的运行开销，这样做很有价值.因此，发送查找数据，我们应当异步通知所有子进程来截取任何待查找的数据.

一旦所有子进程接收到了 `stopRecording` 请求，将调用 `callback` ，并且返回一个包含查找数据的文件.

如果 `resultFilePath` 不为空，那么将把查找数据写入其中，否则写入一个临时文件.实际文件路径如果不为空，则将调用 `callback` .

### `contentTracing.startMonitoring(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

开始向所有进程进行监听.(monitoring)

一旦收到可以开始监听的请求，记录将会立马启动并且在子进程是异步记监听的. 当所有的子进程都收到 `startMonitoring` 请求的时候，`callback` 将会被调用.

### `contentTracing.stopMonitoring(callback)`

* `callback` Function

停止对所有子进程的监听.

一旦所有子进程接收到了 `stopMonitoring` 请求，将调用 `callback` .

### `contentTracing.captureMonitoringSnapshot(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

获取当前监听的查找数据.

子进程通常缓存查找数据，并且仅仅将数据截取和发送给主进程.因为如果直接通过 IPC 来发送查找数据的代价昂贵，我们宁愿避免不必要的查找运行开销.因此，为了停止查找，我们应当异步通知所有子进程来截取任何待查找的数据.

一旦所有子进程接收到了 `captureMonitoringSnapshot` 请求，将调用 `callback` ，并且返回一个包含查找数据的文件.

### `contentTracing.getTraceBufferUsage(callback)`

* `callback` Function

通过查找 buffer 进程来获取百分比最大使用量.当确定了TraceBufferUsage 的值确定的时候，就调用 `callback` .

### `contentTracing.setWatchEvent(categoryName, eventName, callback)`

* `categoryName` String
* `eventName` String
* `callback` Function

任意时刻在任何进程上指定事件发生时将调用 `callback` .

### `contentTracing.cancelWatchEvent()`

取消 watch 事件. 如果启动查找，这或许会造成 watch 事件的回调函数 出错.
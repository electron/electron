# 通知

> 创建系统桌面通知

进程: [Main](../glossary.md#main-process)

## 在渲染器进程中使用

如果你想要从渲染器进程显示通知，则应使用 [HTML5 Notification API](../tutorial/notifications.md)

## 类: Notification

> 创建系统桌面通知

进程: [Main](../glossary.md#main-process)

`Notification` 是一个
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

它通过由 `options ` 设置的原生属性创建一个新的 `Notification`.

### 静态方法

`Notification` 类具有以下静态方法:

#### `Notification.isSupported()`

返回 `Boolean` - 无论当前系统是否支持桌面通知

### `new Notification([options])` _实验性_

* `options` 对象
  * `title` 字符串 - 通知的标题，显示在通知窗口的顶部.
  * `subtitle` 字符串 - (可选) 通知的副标题，将显示在标题下方.  _macOS_
  * `body` 字符串 - 通知的正文，将显示在标题或副标题下方.
  * `silent` 布尔 - (可选) 是否在显示通知时发出系统通知提示音.
  * `icon` [NativeImage](native-image.md) - (可选) 通知所使用的图标
  * `hasReply` 布尔 - (可选) 是否在通知中添加内联的回复选项.  _macOS_
  * `replyPlaceholder` 字符串 - (可选) 在内联输入字段中的提示占位符.  _macOS_
  * `sound` 字符串 - (可选) 显示通知时要播放的声音文件的名称.  _macOS_
  * `actions` [NotificationAction[]](structures/notification-action.md) - (可选) 添加到通知中的操作. 请阅读 `NotificationAction` 文档中的可用操作和限制.  _macOS_


### 实例事件

使用 `new Notification` 创建的对象会发出以下事件:

**注意:** 某些事件仅在特定的操作系统上可用，请参照标签标示。

#### 事件: 'show'

返回:

* `event` 事件

当向用户显示通知时发出. 注意这可以被多次触发, 因为通知可以通过 `show()` 方法多次显示.

#### 事件: 'click'

返回:

* `event` 事件

当用户点击通知时发出.

#### 事件: 'close'

返回:

* `event` 事件

当用户手动关闭通知时发出.

在关闭所有通知的情况下，不能保证会发送此事件.

#### 事件: 'reply' _macOS_

返回:

* `event` 事件
* `reply` 字符串 - 用户输入到内联回复字段的字符串.

当用户点击 `hasReply: true` 的通知上的 “回复” 按钮时发出.

#### Event: 'action' _macOS_

返回:

* `event` 事件
* `index` Number - The index of the action that was activated

### 实例方法

使用 `new Notification` 创建的对象具有以下实例方法:

#### `notification.show()`

立即向用户显示通知. 请注意这不同于 HTML5 Notification 的实现, 简单地实例化一个 `new Notification` 不会立即向用户显示, 你需要在操作系统显示之前调用此方法.

### 播放声音

在 macOS 上, 你可以在显示通知时指定要播放的声音的名称. 除了自定义声音文件, 可以使用任意默认声音 (在 “系统偏好设置” > “声音” 下) . 同时确保声音文件被复制到应用程序包下 (例如`YourApp.app/Contents/Resources`), 或以下位置之一:

* `~/Library/Sounds`
* `/Library/Sounds`
* `/Network/Library/Sounds`
* `/System/Library/Sounds`

查看 [`NSSound`](https://developer.apple.com/documentation/appkit/nssound) 文档获取更多信息.

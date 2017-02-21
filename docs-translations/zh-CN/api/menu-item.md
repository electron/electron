# 菜单项

## 类：菜单项

> 向原生的应用菜单和 context 菜单添加菜单项。

进程： [Main](../glossary.md#main-process)

查看 [`Menu`](menu.md) 的示。

### `new MenuItem(options)`

* `options` Object
  * `click` Function (可选) - 当菜单项被点击的时候，使用 `click(menuItem,browserWindow)` 调用。
    * `menuItem` MenuItem
    * `browserWindow` BrowserWindow
    * `event` Event
  * `role` String (可选) - 定义菜单项的行为，在指定 `click` 属性时将会被忽略。
  * `type` String (可选) - 取值 `normal`, `separator`, `submenu`, `checkbox` or `radio`。
  * `label` String - (可选)
  * `sublabel` String - (可选)
  * `accelerator` [Accelerator](accelerator.md) (可选)
  * `icon` ([NativeImage](native-image.md) | String) (可选)
  * `enabled` Boolean (可选) - 如果为 false，菜单项将显示为灰色不可点击。
    unclickable.
  * `visible` Boolean (可选) - 如果为 false，菜单项将完全隐藏。
  * `checked` Boolean (可选) - 只为 `checkbox` 或 `radio` 类型的菜单项。
  * `submenu` (MenuItemConstructorOptions[] | Menu) (可选) - 应当作为 `submenu` 菜单项的特定类型，当它作为 `type: 'submenu'` 菜单项的特定类型时可以忽略。如果它的值不是 `Menu`，将自动转为 `Menu.buildFromTemplate`。
  * `id` String (可选) - 标志一个菜单的唯一性。如果被定义使用，它将被用作这个菜单项的参考位置属性。
  * `position` String (可选) - 定义菜单的具体指定位置信息。

在创建菜单项时，如果有匹配的方法，建议指定 `role` 属性，
而不是试图手动实现在一个 `click` 函数中的行为。
内置的 `role` 行为将提供最好的原生体验。

当使用 `role' 时，`label' 和 `accelerator` 是可选的，默认为
到每个平台的适当值。

`role`属性值可以为：

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `pasteandmatchstyle`
* `selectall`
* `delete`
* `minimize` - 最小化当前窗口
* `close` - 关闭当前窗口
* `quit`- 退出应用程序
* `reload` - 正常重新加载当前窗口
* `forcereload` - 忽略缓存并重新加载当前窗口
* `toggledevtools` - 在当前窗口中切换开发者工具
* `togglefullscreen`- 在当前窗口中切换全屏模式
* `resetzoom` - 将对焦页面的缩放级别重置为原始大小
* `zoomin` - 将聚焦页面缩小10％
* `zoomout` - 将聚焦页面放大10％

在 macOS 上，`role` 还可以有以下值：

* `about` - 匹配 `orderFrontStandardAboutPanel` 行为
* `hide` - 匹配 `hide` 行为
* `hideothers` - 匹配 `hideOtherApplications` 行为
* `unhide` - 匹配 `unhideAllApplications` 行为
* `startspeaking` - 匹配 `startSpeaking` 行为
* `stopspeaking` - 匹配 `stopSpeaking` 行为
* `front` - 匹配 `arrangeInFront` 行为
* `zoom` - 匹配 `performZoom` 行为
* `window` - "Window" 菜单项
* `help` - "Help" 菜单项
* `services` - "Services" 菜单项

当在 macOS 上指定 `role' 时，`label` 和 `accelerator` 是影响MenuItem的唯一的选项
所有其他选项将被忽略。

### 实例属性

`MenuItem` 对象拥有以下属性：

#### `menuItem.enabled`

一个布尔值表示是否启用该项，此属性可以动态改变。

#### `menuItem.visible`

一个布尔值表示是否可见，此属性可以动态改变。

#### `menuItem.checked`

一个布尔值表示是否选中该项，此属性可以动态改变。

`checkbox` 菜单项将在选中和未选中切换 `checked` 属性。

`radio` 菜单项将在选中切换 `checked` 属性，并且
将关闭同一菜单中所有相邻项目的属性。

您可以为其他行为添加一个 `click` 函数。

#### `menuItem.label`

一个表示菜单项可见标签的字符串

#### `menuItem.click`

当 MenuItem 接收到点击事件时触发的函数

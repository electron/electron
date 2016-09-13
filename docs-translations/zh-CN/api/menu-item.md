# 菜单项
菜单项模块允许你向应用或[menu][1]添加选项。

查看[menu][1]例子。

## 类：MenuItem
使用下面的方法创建一个新的 `MenuItem`

###new MenuItem(options)
* `options` Object
  * `click` Function - 当菜单项被点击的时候，使用 `click(menuItem,browserWindow)` 调用
  * `role` String - 定义菜单项的行为，在指定 `click` 属性时将会被忽略
  * `type` String - 取值 `normal`，`separator`，`checkbox`or`radio`
  * `label` String
  * `sublabel` String
  * `accelerator` [Accelerator][2]
  * `icon` [NativeImage][3]
  * `enabled` Boolean
  * `visible` Boolean
  * `checked` Boolean
  * `submenu` Menu - 应当作为 `submenu` 菜单项的特定类型，当它作为 `type: 'submenu'` 菜单项的特定类型时可以忽略。如果它的值不是 `Menu`，将自动转为 `Menu.buildFromTemplate`。
  * `id` String - 标志一个菜单的唯一性。如果被定义使用，它将被用作这个菜单项的参考位置属性。
  * `position` String - 定义给定的菜单的具体指定位置信息。

在创建菜单项时，如果有匹配的方法，建议指定 `role` 属性，不需要人为操作它的行为，这样菜单使用可以给用户最好的体验。


`role`属性值可以为：

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `selectall`
* `minimize` - 最小化当前窗口
* `close` - 关闭当前窗口

在 macOS 上，`role` 还可以有以下值：

* `about` - 匹配 `orderFrontStandardAboutPanel` 行为
* `hide` - 匹配 `hide` 行为
* `hideothers` - 匹配 `hideOtherApplications` 行为
* `unhide` - 匹配 `unhideAllApplications` 行为
* `front` - 匹配 `arrangeInFront` 行为
* `window` - "Window" 菜单项
* `help` - "Help" 菜单项
* `services` - "Services" 菜单项






 [1]:https://github.com/heyunjiang/electron/blob/master/docs-translations/zh-CN/api/menu.md
 [2]:https://github.com/heyunjiang/electron/blob/master/docs/api/accelerator.md
 [3]:https://github.com/heyunjiang/electron/blob/master/docs/api/native-image.md
# globalShortcut

> 当应用程序没有键盘焦点时检测键盘事件。

进程: [Main](../glossary.md#main-process)

`globalShortcut` 模块可以便捷的为你设置（注册/注销）各种自定义操作的快捷键。

**注意：** 使用此模块注册的快捷键是系统全局的(QQ截图那种), 不要在应用模块（app module）响应 `ready`
消息前使用此模块（注册快捷键）。

```javascript
const {app, globalShortcut} = require('electron')

app.on('ready', () => {
  // Register a 'CommandOrControl+X' shortcut listener.
  const ret = globalShortcut.register('CommandOrControl+X', () => {
    console.log('CommandOrControl+X is pressed')
  })

  if (!ret) {
    console.log('registration failed')
  }

  // Check whether a shortcut is registered.
  console.log(globalShortcut.isRegistered('CommandOrControl+X'))
})

app.on('will-quit', () => {
  // Unregister a shortcut.
  globalShortcut.unregister('CommandOrControl+X')

  // Unregister all shortcuts.
  globalShortcut.unregisterAll()
})
```

## Methods

`globalShortcut` 模块包含以下函数:

### `globalShortcut.register(accelerator, callback)`

* `accelerator` [Accelerator](accelerator.md)
* `callback` Function

注册 `accelerator` 快捷键。当用户按下注册的快捷键时将会调用 `callback` 函数。

当 accelerator 已经被其他应用程序占用时，此调用将
默默地失败。这种行为是操作系统的意图，因为它们没有
想要应用程序争取全局快捷键。

### `globalShortcut.isRegistered(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

返回 `Boolean` - 查询 `accelerator` 快捷键是否已经被注册过了，将会返回 `true` 或 `false`。

当 accelerator 已经被其他应用程序占用时，此调用将
默默地失败。这种行为是操作系统的意图，因为它们没有
想要应用程序争取全局快捷键。

### `globalShortcut.unregister(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

注销全局快捷键 `accelerator`。

### `globalShortcut.unregisterAll()`

注销本应用程序注册的所有全局快捷键。

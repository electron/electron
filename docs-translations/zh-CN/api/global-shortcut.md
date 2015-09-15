# global-shortcut

`global-shortcut` 模块可以便捷的为您设置(注册/注销)各种自定义操作的快捷键.

**Note**: 使用此模块注册的快捷键是系统全局的(QQ截图那种), 不要在应用模块(app module)响应 `ready`
消息前使用此模块(注册快捷键).

```javascript
var app = require('app');
var globalShortcut = require('global-shortcut');

app.on('ready', function() {
  // Register a 'ctrl+x' shortcut listener.
  var ret = globalShortcut.register('ctrl+x', function() {
    console.log('ctrl+x is pressed');
  })

  if (!ret) {
    console.log('registration failed');
  }

  // Check whether a shortcut is registered.
  console.log(globalShortcut.isRegistered('ctrl+x'));
});

app.on('will-quit', function() {
  // Unregister a shortcut.
  globalShortcut.unregister('ctrl+x');

  // Unregister all shortcuts.
  globalShortcut.unregisterAll();
});
```

## Methods

`global-shortcut` 模块包含以下函数:

### `globalShortcut.register(accelerator, callback)`

* `accelerator` [Accelerator](accelerator.md)
* `callback` Function

注册 `accelerator` 快捷键. 当用户按下注册的快捷键时将会调用 `callback` 函数.

### `globalShortcut.isRegistered(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

查询 `accelerator` 快捷键是否已经被注册过了,将会返回 `true`(已被注册) 或 `false`(未注册).

### `globalShortcut.unregister(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

注销全局快捷键 `accelerator`.

### `globalShortcut.unregisterAll()`

注销本应用注册的所有全局快捷键.

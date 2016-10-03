# Menu

`menu`クラスは、アプリケーションのメニューと[コンテキストメニュー](https://developer.mozilla.org/en-US/docs/Mozilla/Tech/XUL/PopupGuide/ContextMenus)として使えるネイティブメニューを作成するのに使われます。このモジュールは、`remote`モジュール経由でレンダラープロセスで使用できるメインプロセスのモジュールです。

個々のメニューは複数の[menu items](menu-item.md)で成り立ち、個々のメニューアイテムはサブメニューを持てます。

以下は、ユーザーはページを右クリックした時、[remote](remote.md)モジュールを作成するために、（レンダラープロセス）ウェブページで動的にメニューを作成して、表示します。

```html
<!-- index.html -->
<script>
const remote = require('electron').remote;
const Menu = remote.Menu;
const MenuItem = remote.MenuItem;

var menu = new Menu();
menu.append(new MenuItem({ label: 'MenuItem1', click: function() { console.log('item 1 clicked'); } }));
menu.append(new MenuItem({ type: 'separator' }));
menu.append(new MenuItem({ label: 'MenuItem2', type: 'checkbox', checked: true }));

window.addEventListener('contextmenu', function (e) {
  e.preventDefault();
  menu.popup(remote.getCurrentWindow());
}, false);
</script>
```

シンプルなテンプレートAPIでレンダラープロセスでアプリケーションメニューを作成する例です。

```javascript
var template = [
  {
    label: 'Edit',
    submenu: [
      {
        label: 'Undo',
        accelerator: 'CmdOrCtrl+Z',
        role: 'undo'
      },
      {
        label: 'Redo',
        accelerator: 'Shift+CmdOrCtrl+Z',
        role: 'redo'
      },
      {
        type: 'separator'
      },
      {
        label: 'Cut',
        accelerator: 'CmdOrCtrl+X',
        role: 'cut'
      },
      {
        label: 'Copy',
        accelerator: 'CmdOrCtrl+C',
        role: 'copy'
      },
      {
        label: 'Paste',
        accelerator: 'CmdOrCtrl+V',
        role: 'paste'
      },
      {
        label: 'Select All',
        accelerator: 'CmdOrCtrl+A',
        role: 'selectall'
      }
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'CmdOrCtrl+R',
        click: function (item, focusedWindow) {
          if (focusedWindow)
            focusedWindow.reload()
        }
      },
      {
        label: 'Toggle Full Screen',
        accelerator: (function () {
          if (process.platform == 'darwin')
            return 'Ctrl+Command+F'
          else
            return 'F11'
        })(),
        click: function (item, focusedWindow) {
          if (focusedWindow)
            focusedWindow.setFullScreen(!focusedWindow.isFullScreen())
        }
      },
      {
        label: 'Toggle Developer Tools',
        accelerator: (function () {
          if (process.platform == 'darwin')
            return 'Alt+Command+I'
          else
            return 'Ctrl+Shift+I'
        })(),
        click: function (item, focusedWindow) {
          if (focusedWindow)
            focusedWindow.toggleDevTools()
        }
      }
    ]
  },
  {
    label: 'Window',
    role: 'window',
    submenu: [
      {
        label: 'Minimize',
        accelerator: 'CmdOrCtrl+M',
        role: 'minimize'
      },
      {
        label: 'Close',
        accelerator: 'CmdOrCtrl+W',
        role: 'close'
      }
    ]
  },
  {
    label: 'Help',
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click: function () { require('electron').shell.openExternal('http://electron.atom.io') }
      }
    ]
  }
]

if (process.platform == 'darwin') {
  var name = require('electron').remote.app.getName()
  template.unshift({
    label: name,
    submenu: [
      {
        label: 'About ' + name,
        role: 'about'
      },
      {
        type: 'separator'
      },
      {
        label: 'Services',
        role: 'services',
        submenu: []
      },
      {
        type: 'separator'
      },
      {
        label: 'Hide ' + name,
        accelerator: 'Command+H',
        role: 'hide'
      },
      {
        label: 'Hide Others',
        accelerator: 'Command+Alt+H',
        role: 'hideothers'
      },
      {
        label: 'Show All',
        role: 'unhide'
      },
      {
        type: 'separator'
      },
      {
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function () { app.quit() }
      }
    ]
  })
  // Window menu.
  template[3].submenu.push(
    {
      type: 'separator'
    },
    {
      label: 'Bring All to Front',
      role: 'front'
    }
  )
}

var menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)
```

## クラス: Menu

### `new Menu()`

新しいメニューを作成します。

## メソッド

`menu`クラスーは次のメソッドを持ちます。

### `Menu.setApplicationMenu(menu)`

* `menu` Menu

macOSで、アプリケーションメニューとして`menu`を設定します。WindowsとLinuxでは、`menu`はそれぞれのウィンドウの上のメニューとして設定されます。

### `Menu.sendActionToFirstResponder(action)` _macOS_

* `action` String

アプリケーションの最初のレスポンダーに`action`が送信されます。規定のCocoaメニュー動作をエミュレートするために使われ、通常は`MenuItem`の`role`プロパティーを使います。

### `Menu.buildFromTemplate(template)`

* `template` Array

一般的に、`template`は、[MenuItem](menu-item.md)を組み立てるための `options`配列です。使用方法は下のように参照します。

ほかのフィールドに`template`の項目を設定でき、メニューアイテムを構成するプロパティです。

### `Menu.popup([browserWindow, x, y, positioningItem])`

* `browserWindow` BrowserWindow (オプション) - 既定では`null`です。
* `x` Number (オプション) - 既定では -1です。
* `y` Number (**必須** `x` が使われている場合) - 既定では -1です。
* `positioningItem` Number (オプション) _macOS_ - 既定では -1です。

メニューアイテムのインデックスを指定した座標にマウスカーソルを配置します。

`browserWindow`でコンテキストメニューとしてメニューをポップアップします。メニューを表示する場所をオプションで`x, y`座標を指定でき、指定しなければ現在のマウスカーソル位置に表示します。

### `Menu.append(menuItem)`

* `menuItem` MenuItem

メニューに`menuItem`を追加します。

### `Menu.insert(pos, menuItem)`

* `pos` Integer
* `menuItem` MenuItem

メニューの`pos`位置に`menuItem`を追加します。

### `Menu.items()`

メニューのアイテムを収容した配列を取得します。

## macOS アプリケーションメニューの注意事項

macOSは、WindowsとLinuxのアプリケーションのメニューとは完全に異なるスタイルを持ち、よりネイティブのようにアプリメニューを作成するのに幾つかの注意事項があります。

### 標準的なメニュー

macOSでは、`Services`と`Windows`メニューのように定義された標準的な多くのメニューがあります。標準的なメニューを作成するために、メニューの`role`に次のどれかを設定する必要があり、Electronはそれを受けて標準的なメニューを作成します。

* `window`
* `help`
* `services`

### 標準的なメニューアイテムの動作

`About xxx`と`Hide xxx`、`Hide Others`のようないくつかのメニューアイテム用にmacOSは標準的な動作を提供します。メニューアイテムの動作に標準的な動作を設定するために、メニューアイテムの`role`属性を設定すべきです。

### メインのメニュー名

macOSでは、設定したラベルに関係なく、アプリケーションの最初のアイテムのラベルはいつもアプリの名前です。変更するために、アプリにバンドルされている`Info.plist`ファイルを修正してアプリの名前を変更する必要があります。詳細は、 [About Information Property List Files][AboutInformationPropertyListFiles] を見てください。

## メニューアイテムの位置

`position`を使用することができ、`Menu.buildFromTemplate`でメニューを構築するときに`id`がアイテムを配置する方法をコントロールします。

`MenuItem`の`position`属性は、`[placement]=[id]`をもち、 `placement`は`before`や `after`、 `endof`の一つが設定され、`id`はメニューの設定されているアイテムで一意のIDです。

* `before` - IDから参照したアイテムの前にアイテムを挿入します。参照するアイテムが存在しないのなら、メニューの最後にアイテムが挿入されます。
* `after` - IDから参照したアイテムの後にアイテムを挿入します。参照するアイテムが存在しないのなら、メニューの最後にアイテムが挿入されます。
* `endof` -IDから参照したアイテムを含む論理グループの最後にアイテムを挿入します（グループはアイテムを分けるために作成されます）。参照するアイテムが存在しないのなら、付与されたIDで新しい分離グループが作成され、そのグループのあとにアイテムが挿入されます。

アイテムが配置されたとき、新しいアイテムが配置されるまで、すべての配置されていないアイテムがその後に挿入されます。同じ場所にメニューアイテムのグループを配置したいのなら、最初にアイテムで場所を指定する必要があります。

### 具体例

テンプレート:

```javascript
[
  {label: '4', id: '4'},
  {label: '5', id: '5'},
  {label: '1', id: '1', position: 'before=4'},
  {label: '2', id: '2'},
  {label: '3', id: '3'}
]
```

メニュー:

```
- 1
- 2
- 3
- 4
- 5
```

テンプレート:

```javascript
[
  {label: 'a', position: 'endof=letters'},
  {label: '1', position: 'endof=numbers'},
  {label: 'b', position: 'endof=letters'},
  {label: '2', position: 'endof=numbers'},
  {label: 'c', position: 'endof=letters'},
  {label: '3', position: 'endof=numbers'}
]
```

メニュー:

```
- ---
- a
- b
- c
- ---
- 1
- 2
- 3
```

[AboutInformationPropertyListFiles]: https://developer.apple.com/library/ios/documentation/general/Reference/InfoPlistKeyReference/Articles/AboutInformationPropertyListFiles.html

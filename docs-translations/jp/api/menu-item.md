# MenuItem

`menu-item`モジュールは、アプリケーションまたはコンテキスト[`メニュー`](menu.md)に項目を追加することができます。

具体例は、 [`menu`](menu.md) を見てください。

## クラス: MenuItem

次のメソッドで、新しい`MenuItem`を作成します。

### new MenuItem(options)

* `options` Object
  * `click` Function - メニューアイテムがクリックされたとき、`click(menuItem, browserWindow)`がコールされます。
  * `role` String - 指定されたとき、メニューアイテムの動作が定義され、`click`プロパティは無視されます。
  * `type` String - `normal`と `separator`、`submenu`、`checkbox`、`radio`を指定できます。
  * `label` String
  * `sublabel` String
  * `accelerator` [Accelerator](accelerator.md)
  * `icon` [NativeImage](native-image.md)
  * `enabled` Boolean
  * `visible` Boolean
  * `checked` Boolean
  * `submenu` Menu - メニューアイテムを省略できる`type: 'submenu'`を指定したとき、`submenu`種類のメニューアイテムを指定すべきです。値が`Menu`でないとき、`Menu.buildFromTemplate`を使用して自動的に変換されます。
  * `id` String - 1つのメニュー内で一意です。定義されていたら、position属性によってアイテムへの参照として使用できます。
  * `position` String - このフィールドは、指定されたメニュー内の特定の位置を細かく定義できます。

メニューアイテムを作成するとき、適切な動作がある場合は、メニューでベストな自然な体験を提供するために、手動で実装する代わりに`role`を指定することを推奨します。

`role` プロパティは次の値を持ちます:

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `selectall`
* `minimize` - 現在のウィンドウの最小化
* `close` - 現在のウィンドウを閉じます

macOSでは、`role`は次の追加の値を取れます:

* `about` - `orderFrontStandardAboutPanel`動作に紐づけられます
* `hide` - `hide`動作に紐づけられます
* `hideothers` - `hideOtherApplications`動作に紐づけられます
* `unhide` - `unhideAllApplications`動作に紐づけられます
* `front` - `arrangeInFront`動作に紐づけられます
* `window` - サブメニューは "Window"メニューです。
* `help` - サブメニューは "Help"メニューです。
* `services` - サブメニューは "Services"メニューです。

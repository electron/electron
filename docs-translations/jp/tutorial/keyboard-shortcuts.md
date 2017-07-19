# Keyboard Shortcuts

> ローカル、グローバルのキーボードショートカットを設定します

## ローカルショートカット

[Menu] モジュールの設定により、アプリケーションにフォーカスがあるときのキーボードショートカットを設定できます。
[MenuItem] を作成するときの [`accelerator`] プロパティで設定します。

```js
const {Menu, MenuItem} = require('electron')
const menu = new Menu()

menu.append(new MenuItem({
  label: 'Print',
  accelerator: 'CmdOrCtrl+P',
  click: () => { console.log('time to print stuff') }
}))
```

キーの組み合わせをユーザのOSに基づいて変えることも簡単です。

```js
{
  accelerator: process.platform === 'darwin' ? 'Alt+Cmd+I' : 'Ctrl+Shift+I'
}
```

## グローバルショートカット

[globalShortcut] モジュールを使うことで、アプリケーションにフォーカスがないときのキーボードイベントを検知することができます。

```js
const {app, globalShortcut} = require('electron')

app.on('ready', () => {
  globalShortcut.register('CommandOrControl+X', () => {
    console.log('CommandOrControl+X is pressed')
  })
})
```

## BrowserWindow でのキーボードショートカット

もしキーボードショートカットイベントを [BrowserWindow] でハンドリングしたい場合は、rendererプロセスの中でwindowオブジェクトの`keyup` と `keydown` のイベントリスナーを使ってください。

```js
window.addEventListener('keyup', doSomething, true)
```

第3引数にtrueを指定した場合には、リスナーは常に他のリスナーが起動する前にキーイベントを受け取るようになります。そのため、 `stopPropagation()` を呼び出すことができないことに注意してください。

[`before-input-event`](web-contents.md#event-before-input-event) イベントは表示しているページの `keydown` and `keyup` イベントが発生する前に発行されます。
メニューに表示がないショートカットも補足することができます。

もし自前でショートカットキーの判定を実装したくない場合には、[mousetrap] のような高度なキー検出を行うライブラリーがあります。

```js
Mousetrap.bind('4', () => { console.log('4') })
Mousetrap.bind('?', () => { console.log('show shortcuts!') })
Mousetrap.bind('esc', () => { console.log('escape') }, 'keyup')

// combinations
Mousetrap.bind('command+shift+k', () => { console.log('command shift k') })

// map multiple combinations to the same callback
Mousetrap.bind(['command+k', 'ctrl+k'], () => {
  console.log('command k or control k')

  // return false to prevent default behavior and stop event from bubbling
  return false
})

// gmail style sequences
Mousetrap.bind('g i', () => { console.log('go to inbox') })
Mousetrap.bind('* a', () => { console.log('select all') })

// konami code!
Mousetrap.bind('up up down down left right left right b a enter', () => {
  console.log('konami code')
})
```

[Menu]: ../api/menu.md
[MenuItem]: ../api/menu-item.md
[globalShortcut]: ../api/global-shortcut.md
[`accelerator`]: ../api/accelerator.md
[BrowserWindow]: ../api/browser-window.md
[mousetrap]: https://github.com/ccampbell/mousetrap

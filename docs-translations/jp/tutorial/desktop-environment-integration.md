# デスクトップ環境の統合

異なるオペレーティングシステムは、それぞれのデスクトップ環境でデスクトップに統合されたアプリケーション用に異なる機能を提供します。例えば、Windows アプリケーションではタスクバーのジャンプバーリストにショートカットをおけ、Macではドックメニューにカスタムメニューをおけます。

このガイドでは、Electron APIでデスクトップ環境にアプリケーションを統合する方法を説明します。

## 通知 (Windows, Linux, macOS)

3つのオペレーティングシステム全てで、アプリケーションからユーザーに通知を送る手段が提供されています。通知を表示するためにオペレーティングシステムのネイティブ通知APIを使用しする[HTML5 Notification API](https://notifications.spec.whatwg.org/)で、Electronは、開発者に通知を送ることができます。

**注意:** これはHTML5 APIですので、レンダラプロセス内のみで有効です。


```javascript
let myNotification = new Notification('Title', {
  body: 'Lorem Ipsum Dolor Sit Amet'
})

myNotification.onclick = () => {
  console.log('Notification clicked')
}
```

オペレーティングシステム間でコードとユーザ体験は似ていますが、細かい違いがあります。

### Windows

* Windows 10では、通知はすぐに動作します。
* Windows 8.1 と Windows 8では、[Application User
Model ID][app-user-model-id]で、アプリへのショートカットはスタートメニューにインストールされます。しかし、スタートメニューにピン止めをする必要がありません。
* Windows 7以前は、通知はサポートされていません。 しかし、[Tray API][tray-balloon]を使用してバルーンヒントを送信することができます。

その上で、bodyの最大サイズは250文字であることに留意してください。Windowsチームは、通知は200文字にすることを推奨しています。

### Linux

通知は、`libnotify`を使用して送信されます。[デスクトップ通知仕様][notification-spec]に対応したデスクトップ環境上（Cinnamon、Enlightenment、Unity、GNOME、KDEなど）で通知を表示できます。

### macOS

通知は、そのままmacOSに通知されます。しかし、[通知に関するAppleのヒューマンインターフェイスガイドライン（英語版）](https://developer.apple.com/library/mac/documentation/UserExperience/Conceptual/OSXHIGuidelines/NotificationCenter.html)を知っておくべきです。

通知は、256バイトサイズに制限されており、制限を超えていた場合、通知が破棄されます。

## 最近のドキュメント (Windows と macOS)

Windows と macOSは、ジャンプリストやドックメニュー経由で、アプリケーションが開いた最近のドキュメント一覧に簡単にアクセスできます。

__JumpList:__

![JumpList Recent Files](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__Application dock menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

最近のドキュメントにファイルを追加するために、[app.addRecentDocument][addrecentdocument] APIを使用できます:

```javascript
app.addRecentDocument('/Users/USERNAME/Desktop/work.type')
```

[app.clearRecentDocuments][clearrecentdocuments] API を使用して、最近のドキュメント一覧を空にできます:

```javascript
app.clearRecentDocuments()
```

### Windows 留意点

Windows で、この機能を使用できるようにするために、アプリケーションにドキュメントのファイルタイプのハンドラーを登録すべきです。さもなければ、ジャンプリストに表示されません。[Application Registration][app-registration]で、登録しているアプリケーションをすべて見れます。

ユーザーがジャンプリストからファイルをクリックしたとき、アプリケーションの新しいインスタンスは、コマンドライン引数にファイルのパスを渡して開始します。

### macOS 留意点

ファイルが最近のドキュメントメニューからリクエストされた時、 `app` モジュールの `open-file` イベントが発行されます。

## ドックメニュー (macOS)のカスタマイズ

通常アプリケーションで使用する共通機能用のショートカットを含める、ドック用のカスタムメニューをmacOSでは指定できます。

__Dock menu of Terminal.app:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

カスタムドックメニューを設定するために、macOSのみに提供されている `app.dock.setMenu` APIを使用できます。

```javascript
const electron = require('electron')
const app = electron.app
const Menu = electron.Menu

const dockMenu = Menu.buildFromTemplate([
  { label: 'New Window', click () { console.log('New Window') } },
  { label: 'New Window with Settings', submenu: [
    { label: 'Basic' },
    { label: 'Pro'}
  ]},
  { label: 'New Command...'}
])
app.dock.setMenu(dockMenu)
```

## ユーザータスク (Windows)

Windowsでは、ジャンプリストの `Tasks` カテゴリでカスタムアクションを指定できます。
MSDNから引用します。

>アプリケーションでは、プログラムの機能とユーザーがプログラムを使用して実行する可能性が最も高い主な操作の両方に基づいてタスクを定義します。タスクを実行するためにアプリケーションが起動している必要がないように、タスクは状況に依存しないものにする必要があります。また、タスクは、一般的なユーザーがアプリケーションで実行する操作の中で、統計上最も一般的な操作です。たとえば、メール プログラムでは電子メールの作成や予定表の表示、ワード プロセッサでは新しい文書の作成、特定のモードでのアプリケーションの起動、アプリケーションのサブコマンドを実行することなどです。一般的なユーザーが必要としない高度な機能や、登録などの 1 回限りの操作によって、メニューがわかりづらくなることがないようにしてください。アップグレードやキャンペーンなどの販売促進用の項目としてタスクを使用しないでください。

>タスク一覧は静的なものにすることを強くお勧めします。アプリケーションの状態や状況に関係なく、タスク一覧は同じ状態にすることをお勧めします。リストを動的に変更することも可能ですが、ユーザーはターゲット一覧のこの部分が変更されると考えていないので、ユーザーを混乱させる可能性があることを考慮してください。

__Internet Explorerのタスク:__

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

実際のメニューであるmacOSのドックメニューとは異なり、ユーザーがタスクをクリックしたとき、Windowsではユーザータスクはアプリケーションのショートカットのように動作し、プログラムは指定された引数を実行します。

アプリケーション用のユーザータスクを設定するために、[app.setUserTasks][setusertaskstasks] APIを使用できます:

```javascript
app.setUserTasks([
  {
    program: process.execPath,
    arguments: '--new-window',
    iconPath: process.execPath,
    iconIndex: 0,
    title: 'New Window',
    description: 'Create a new window'
  }
])
```

タスクリストをクリアするために、`app.setUserTasks` をコールし、配列を空にします。

```javascript
app.setUserTasks([])
```

アプリケーションを閉じた後もユーザータスクは表示されていてるので、アプリケーションをアンインストールするまではタスクに指定したアイコンとプログラムのパスは存在し続けてる必要があります。

## サムネイルツールバー（縮小表示ツールバー）

Windowsでは、アプリケーションウィンドウのタスクバーレイアウトに、指定のボタンを縮小表示ツールバーに追加できます。アプリケーションのウィンドウを元のサイズに戻したりアクティブ化したりすることなく、主要なコマンドにアクセスできるようにします。

MSDNからの引用:

>このツール バーは、単純に、使い慣れた標準的なツール バー コモン コントロールです。最大で 7 個のボタンが配置されます。各ボタンの ID、イメージ、ツールヒント、および状態は構造体で定義され、その後タスク バーに渡されます。アプリケーションでは、現在の状態での必要に応じて、縮小表示ツール バーのボタンの表示、有効化、無効化、非表示を実行できます。
>たとえば、Windows Media Player の縮小表示ツール バーでは、再生、一時停止、ミュート、停止などの、標準的なメディア トランスポート コントロールを提供できます。

__Windows Media Playerの縮小表示ツールバー:__

![player](https://i-msdn.sec.s-msft.com/dynimg/IC420540.png)

アプリケーションに縮小表示ツールバーを設定するために、[BrowserWindow.setThumbarButtons][setthumbarbuttons]を使えます:

```javascript
const {BrowserWindow} = require('electron')
const path = require('path')

let win = new BrowserWindow({
  width: 800,
  height: 600
})
win.setThumbarButtons([
  {
    tooltip: 'button1',
    icon: path.join(__dirname, 'button1.png'),
    click () { console.log('button2 clicked') }
  },
  {
    tooltip: 'button2',
    icon: path.join(__dirname, 'button2.png'),
    flags: ['enabled', 'dismissonclick'],
    click () { console.log('button2 clicked.') }
  }
])
```

縮小表示ツールバーボタンをクリアするために、 `BrowserWindow.setThumbarButtons` をコールして配列を空にします：

```javascript
win.setThumbarButtons([])
```

## Unity ランチャーショートカット (Linux)

Unityでは、`.desktop` ファイルの修正を経由してランチャーにカスタムエントリーを追加できます。[Adding Shortcuts to a Launcher][unity-launcher]を参照してください。

__Audaciousのランチャーショートカット:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

## タスクバーの進行状況バー (Windows, macOS, Unity)

Windowsでは、タスクバーボタンは、進行状況バーを表示するのに使えます。ウィンドウを切り替えることなくウィンドウの進行状況情報をユーザーに提供することができます。

macOSではプログレスバーはドックアイコンの一部として表示されます。
Unity DEは、ランチャーに進行状況バーの表示をするのと同様の機能を持っています。

__タスクバーボタン上の進行状況バー:__

![Taskbar Progress Bar](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

ウィンドウに進行状況バーを設定するために、[BrowserWindow.setProgressBar][setprogressbar] APIを使えます:

```javascript
let win = new BrowserWindow({...});
win.setProgressBar(0.5);
```

## タスクバーでアイコンをオーバーレイする (Windows)

Windowsで、タスクバーボタンはアプリケーションステータスを表示するために小さなオーバーレイを使うことができます。MSDNから引用します。

> アイコン オーバーレイは、状況に応じた状態通知として機能し、通知領域に状態アイコンを個別に表示する必要性をなくして、情報をユーザーに伝えることを目的としています。たとえば、現在、通知領域に表示される Microsoft Office Outlook の新着メールの通知は、タスク バー ボタンのオーバーレイとして表示できるようになります。ここでも、開発サイクルの間に、アプリケーションに最適な方法を決定する必要があります。アイコン オーバーレイは、重要で長期にわたる状態や通知 (ネットワークの状態、メッセンジャーの状態、新着メールなど) を提供することを目的としています。ユーザーに対して、絶えず変化するオーバーレイやアニメーションを表示しないようにしてください。

__タスクバーボタンでのオーバーレイ:__

![Overlay on taskbar button](https://i-msdn.sec.s-msft.com/dynimg/IC420441.png)

ウィンドウでオーバーレイアイコンを設定するために、[BrowserWindow.setOverlayIcon][setoverlayicon] APIを使用できます。

```javascript
let win = new BrowserWindow({...});
win.setOverlayIcon('path/to/overlay.png', 'Description for overlay');
```

##  Windowのファイル表示 (macOS)

macOSでは、ウィンドウがrepresented fileを設定でき、タイトルバー上にファイルのアイコンを表示でき、タイトル上でCommand-クリックまたはControl-クリックをすると、パスがポップアップ表示されます。

ウィンドウの編集状態を設定できるように、このウィンドウのドキュメントが変更されたかどうかをファイルのアイコンで示せます。

__Represented file ポップアップメニュー:__

<img src="https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png" height="232" width="663" >

ウィンドウにrepresented fileを設定するために、[BrowserWindow.setRepresentedFilename][setrepresentedfilename] と [BrowserWindow.setDocumentEdited][setdocumentedited] APIsを使えます:

```javascript
let win = new BrowserWindow({...});
win.setRepresentedFilename('/etc/passwd');
win.setDocumentEdited(true);
```

[addrecentdocument]: ../api/app.md#appaddrecentdocumentpath-os-x-windows
[clearrecentdocuments]: ../api/app.md#appclearrecentdocuments-os-x-windows
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks-windows
[setprogressbar]: ../api/browser-window.md#winsetprogressbarprogress
[setoverlayicon]: ../api/browser-window.md#winsetoverlayiconoverlay-description-windows-7
[setrepresentedfilename]: ../api/browser-window.md#winsetrepresentedfilenamefilename-os-x
[setdocumentedited]: ../api/browser-window.md#winsetdocumenteditededited-os-x
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
[setthumbarbuttons]: ../api/browser-window.md#winsetthumbarbuttonsbuttons-windows-7
[tray-balloon]: ../api/tray.md#traydisplayballoonoptions-windows
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[notification-spec]: https://developer.gnome.org/notification-spec/

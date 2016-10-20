# アプリケーションのパッケージ化

Windows上の長いパス名周りの[問題](https://github.com/joyent/node/issues/6960) を軽減したり、`require`を若干スピードアップしたり、簡単な調査からソースコードを隠したりするために、ソースコードを少々変更して、アプリケーションを [asar][asar] アーカイブとしてパッケージ化することもできます。

## `asar` アーカイブの生成

[asar][asar] アーカイブは、１つのファイルに連結されたtarライクのシンプルなフォーマットです。Electron はファイル全体をアンパックしなくても任意のファイルを読み込めます。

アプリを `asar` アーカイブにパッケージ化する手順：

### 1. asar ユーティリティのインストール

```bash
$ npm install -g asar
```

### 2. `asar pack`でパッケージ化する

```bash
$ asar pack your-app app.asar
```

## `asar` アーカイブを使用する

Electronには、2組のAPIがあります。Node.js により提供される Node API、そして Chromium により提供される Web API です。どちらの API も `asar` アーカイブからのファイル読み込みに対応しています。

### Node API

Electronでは特定のパッチで、`fs.readFile` や `require` のようなNode APIは、`asar` アーカイブを仮想ディレクトリのように扱い、ファイルをファイルシステム上の通常のファイルのように扱います。

例えば、`/path/to` 配下に、`example.asar` アーカイブがあると仮定します:

```bash
$ asar list /path/to/example.asar
/app.js
/file.txt
/dir/module.js
/static/index.html
/static/main.css
/static/jquery.min.js
```

`asar` アーカイブ内のファイルを読み込む:

```javascript
const fs = require('fs')
fs.readFileSync('/path/to/example.asar/file.txt')
```

アーカイブのルート直下にあるすべてのファイルを一覧にする:

```javascript
const fs = require('fs')
fs.readdirSync('/path/to/example.asar')
```

アーカイブからモジュールを使用する:

```javascript
require('/path/to/example.asar/dir/module.js')
```

`BrowserWindow` を使って `asar` アーカイブ内の Web ページを表示することもできます:

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('file:///path/to/example.asar/static/index.html')
```

### Web API

Webページで、アーカイブ内のファイルを `file:` プロトコルでリクエストできます。
Node API と同様、`asar` アーカイブはディレクトリのように扱われます。

例えば、 `$.get` でファイルを取得できます:

```html
<script>
var $ = require('./jquery.min.js');
$.get('file:///path/to/example.asar/file.txt', (data) => {
  console.log(data);
});
</script>
```

### `asar` アーカイブを通常のファイルのように扱う

アーカイブのチェックサムを検証する等の幾つかのケースでは、`asar` アーカイブをファイルとして読み込む必要があります。この目的のために、 `asar` サポートしないオリジナルの `fs` API を提供するビルトインの `original-fs` モジュールを使用できます。

```javascript
const originalFs = require('original-fs')
originalFs.readFileSync('/path/to/example.asar')
```

`process.noAssar` に `true` をセットしても `fs` モジュールの `asar` サポートを無効にすることができます：

```javascript
process.noAsar = true
fs.readFileSync('/path/to/example.asar')
```

## Node API の制限

Node APIで、`asar` アーカイブが可能な限りディレクトリのように動作するよう懸命に試してますが、低レベル環境での Node API に起因した制限が幾つかあります。

### アーカイブは読み取り専用です

アーカイブは修正できないため、ファイルを変更できる変更できる全ての Node API は `asar` アーカイブに対して動作しません。

### 作業ディレクトリは、アーカイブ内のディレクトリに設定できません

`asar` アーカイブはディレクトリのように扱われるにも関わらず、ファイルシステム上には実際のディレクトリが存在しないため、`asar` アーカイブ内のディレクトリを作業ディレクトリとして設定することはできません。幾つかの API の `cwd` オプションにアーカイブ内のディレクトリを渡すのも同様にエラーの原因になります。

### いくつかのAPIで追加のアンパッキングがされます

たいていの `fs` APIは、アンパッキングせずに、 `asar` アーカイブからファイルを読み込んだり、ファイル情報を取得できます。しかし、システムコールに実際のファイルパスを渡すようになっている幾つかの API では、Electron は必要なファイルを一時ファイルとして抽出し、API に一時ファイルのパスを渡して、API が動作するようにします。このため、当該 API には多少のオーバーヘッドがあります。

追加のアンパッキングに必要なAPIです:

* `child_process.execFile`
* `child_process.execFileSync`
* `fs.open`
* `fs.openSync`
* `process.dlopen` - ネイティブモジュール上の `require` で使用されます

### `fs.stat` の偽の統計情報

`asar` アーカイブ内のファイルはファイルシステム上に存在しないので、`fs.stat` および `asar` アーカイブ内のファイルへの関連情報によって返される`Stats` オブジェクトは、推測して生成されます。ファイルサイズの取得とファイルタイプのチェックを除いて、 `Stats` オブジェクトを信頼すべきではありません。

### `asar` アーカイブ内のバイナリの実行

`child_process.exec` と `child_process.spawn` 、 `child_process.execFile` のようなバイナリを実行できるNode APIがあります。しかし、`execFile` だけが、`asar` アーカイブ内でのバイナリ実行をサポートしています。

なぜならば、`exec` と `spawn` は入力として `file` の代わりに `command` を受け取り、`command` はシェル配下で実行されるからです。コマンドが asar アーカイブ内のファイルを使うかどうかを決定するための信頼できる方法はありませんし、そうするとしてもコマンドで使うファイルパスを副作用なしに置き換えることができるかどうかを確認することはできません。

## `asar` アーカイブ内のファイルをアンパックして追加

上記のように、いくつかのNode APIが呼ばれると、ファイルシステム上にファイルをアンパックしますが，パフォーマンス問題は別として、ウィルススキャナーのアラートにつながる可能性があります。

これに対応するために、`--unpack` オプションを使用して、アーカイブを作成する際に、いくつかのファイルをアンパックできます。例えば、ネイティブモジュールの共有ライブラリを除く場合：

```bash
$ asar pack app app.asar --unpack *.node
```

このコマンドを実行した後、`app.asar` とは別に、アンパックされたファイルを含んだ`app.asar.unpacked` フォルダーが生成されます。ユーザーに提供するときには、`app.asar` と一緒にコピーしなければなりません。

[asar]: https://github.com/electron/asar

# アプリケーションのパッケージ化

Windows上の長いパス名周りの[issues](https://github.com/joyent/node/issues/6960) を軽減させるたり、`require`を若干スピードアップさせたり、 簡単な調査からソースコードを隠したりするために、ソースコードの少しの変更で、[asar][asar]にアーカイブし、パッケージ化を選択することもできます。

## `asar` アーカイブの生成

[asar][asar] アーカイブは、１つのファイルに連結されたtarライクのシンプルなフォーマットです。Electronは、すべてのファイルをアンパッキングせずに、任意のファイルを読み込めます。

アプリを `asar` アーカイブにパッケージ化する手順

### 1. asar ツールのインストール

```bash
$ npm install -g asar
```

### 2. `asar pack`でパッケージ化する

```bash
$ asar pack your-app app.asar
```

## `asar` アーカイブを使用する

Electronには、2組のAPIがあります。Node APIは、Node.jsで提供されており、Web APIは、Chromiumで提供されています。両方のAPIは、 `asar` アーカイブからのファイル読み込みに対応しています。

### Node API

Electronでは特定のパッチで、`fs.readFile` や `require` のようなNode APIは、`asar` アーカイブを仮想ディレクトリのように扱い、ファイルをファイルシステムで通常のファイルのように扱います。

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

`asar` アーカイブでファイルを読み込む:

```javascript
const fs = require('fs');
fs.readFileSync('/path/to/example.asar/file.txt');
```

アーカイブのルート直下にあるすべてのファイルを一覧にする:

```javascript
const fs = require('fs');
fs.readdirSync('/path/to/example.asar');
```

アーカイブからモジュールを使用する:

```javascript
require('/path/to/example.asar/dir/module.js');
```

 `BrowserWindow`で、`asar`アーカイブで、Webページを表示することができます:

```javascript
const BrowserWindow = require('electron').BrowserWindow;
var win = new BrowserWindow({width: 800, height: 600});
win.loadURL('file:///path/to/example.asar/static/index.html');
```

### Web API

Webページで、アーカイブ内のファイルを `file:` プロトコルでリクエストできます。
Node APIのように、`asar` アーカイブはディレクトリのように扱われます。

例えば、 `$.get` でファイルを取得できます:

```html
<script>
var $ = require('./jquery.min.js');
$.get('file:///path/to/example.asar/file.txt', function(data) {
  console.log(data);
});
</script>
```

### `asar` アーカイブを通常のファイルのように扱う

For some cases like verifying the `asar` アーカイブのチェックサムを検証するようなケースで、 `asar` アーカイブのコンテンツをファイルのように扱う必要があります。この目的のために、 `asar` サポートしないオリジナルの `fs` API が提供する ビルトインの　`original-fs` モジュールを使用できます。

```javascript
var originalFs = require('original-fs');
originalFs.readFileSync('/path/to/example.asar');
```

`fs` モジュールで、`asar` サポートを無効化するために、`process.noAsar` を `true` に設定します:

```javascript
process.noAsar = true;
fs.readFileSync('/path/to/example.asar');
```

## Node API の制限

Node APIで、`asar` アーカイブが可能な限りディレクトリのように動作するよう懸命に試してますが、Node APIの低レベル性質の影響で、まだ制限があります。

### アーカイブは読み取り専用です

ファイルを変更できる全てのNode APIは、 `asar` アーカイブに対して動作しないので、アーカイブは変更されません。

### 作業ディレクトリは、アーカイブ内のディレクトリに設定できません

`asar` アーカイブはディレクトリのように扱われるにも関わらず、ファイルシステム上では実際のディレクトリではなく、`asar` アーカイブ内に決して作業ディレクトリは設定されません。いくつかのAPIの`cwd`オプションを設定していて、それがエラー原因になります。

### いくつかのAPIで追加のアンパッキングがされます

たいていの `fs` APIは、アンパッキングせずに、 `asar` アーカイブからファイルを読み込んだり、ファイル情報を取得できます。しかし、実際のファイルパスを通してシステムコールに依存するAPIもあり、Electronは必要なファイルを一時ファイルに解凍し、一時ファイルのパスを通して、APIが動作します。これは、APIに多少のオーバーヘッドがあります。

追加のアンパッキングに必要なAPIです。:

* `child_process.execFile`
* `child_process.execFileSync`
* `fs.open`
* `fs.openSync`
* `process.dlopen` - ネイティブモジュール上の `require` で使用されます

### `fs.stat` の偽の統計情報

ファイルシステム上にファイルが存在しないので、`fs.stat` および `asar` アーカイブ内のファイルへの関連情報によって返される`Stats` オブジェクトは、推測して生成されます。ファイルサイズの取得とファイルタイプのチェックを除いて、 `Stats` オブジェクトを信頼すべきではありません。

### `asar` アーカイブ内でバイナリを実行します

`child_process.exec` と `child_process.spawn` 、 `child_process.execFile` のようなバイナリを実行できるNode APIがあります。しかし、`execFile` だけが、`asar` アーカイブ内でのバイナリ実行をサポートしています。

`exec` と `spawn` は、インプットを `file` の代わりに `command` を受け取り、`command` はシェル配下で実行されます。コマンドがasar アーカイブ内のファイルを使うかどうかを決定するための信頼できる方法はありません。影響なしで、コマンドでパスを置き換えることができるかどうかを確認することはできません。

## アンパッキングする `asar` アーカイブ内のファイルを追加する

上記のように、いくつかのNode APIは、呼び出さすとき、ファイルをファイルパスへアンパックします。パフォーマンス問題は別として、ウィルススキャナーのアラートにつながる可能性があります。

これに対する対応として、`--unpack` オプションを使用して、アーカイブを作成する際に、いくつかのファイルをアンパックできます。例えば、ネイティブモジュールの共有ライブラリを除きます。

```bash
$ asar pack app app.asar --unpack *.node
```

このコマンドを実行した後、`app.asar` とは別に、アンパックファイルを含んだ`app.asar.unpacked` フォルダーが生成され、ユーザーに提供するときには、`app.asar` と一緒にコピーしなければなりません。

[asar]: https://github.com/atom/asar

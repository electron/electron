# Electron と NW.js (node-webkit)の技術的違い

__Note: Electron は以前は Atom Shell と呼ばれていました__

NW.js と同じように Electron は HTML と JavaScript でデスクトップアプリケーションを書くためのプラットフォームを提供しており、また web ページからローレベルなシステムにもアクセスできる Node との統合的な仕組みを持っていました。

しかし、2つのプロジェクトには根本的な違いがあり、Electron と NW.js は完全に別なプロダクトになりました。

__1. アプリケーションの開始__

NW.js ではアプリケーションは web ページから開始されます。 `package.json` にてアプリケーションのメインページURLを指定し、ブラウザウィンドウがそのページを開くことで、それがアプリケーションのメインウィンドウとなります。

Electron では、JavaScript のスクリプトがエントリポイントとなります。URLを直接指定するのではなく、自分でブラウザウィンドウを作成し、API を通して HTML ファイルを読み込みます。また、ウィンドウで発生するイベントを購読して、アプリケーションの終了をハンドリングする必要もあります。

Electron は Node.js ランタイムのように動作します。 Electron の API はローレベルなので、[PhantomJS](http://phantomjs.org/)の代わりにブラウザテストに使用することもできます。

__2. ビルドシステム__

Chromium の全てのコードをビルドする複雑さを回避するため、Electron は [`libchromiumcontent`](https://github.com/brightray/libchromiumcontent) を通して Chromium の Content API にアクセスします。

`libchromiumcontent` は Chromium の Content モジュールとそれに依存する全てを含んだ単一の共有ライブラリです。
おかげで Electron をビルドするためにパワフルなマシンを用意する必要はありません。

__3. Node との統合__

NW.js では web ページと Node を統合するために Chromium にパッチを適用しています。一方、Electron では Chromium を改造する方法を取らず、libuv ループとプラットフォームのメッセージループを統合する方法を私達は取りました。
何をやっているかについては、[`node_bindings`][node-bindings] を参照してください。

__4. Multi-context__

もしあなたが経験豊かな NW.js ユーザーならば、Node のコンテキストと Web のコンテキストに精通しているかと思います。これらのコンセプトは、NW.js の実装によって導入されたものです。

Node の [multi-context](http://strongloop.com/strongblog/whats-new-node-js-v0-12-multiple-context-execution/) 機能によって、Electron は Web ページに新しい Javascript のコンテキストを導入しません。

Note: NW.js は バージョン 0.13 からオプションとして multi-context をサポートしました。

[node-bindings]: https://github.com/electron/electron/tree/master/atom/common

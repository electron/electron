# powerMonitor

`power-monitor`モジュールは、パワー状態の変更の監視に使われます。メインプロセスでみ使用することができます。`app`モジュールの`ready`が出力されるまで、このモジュールを使うべきではありません。

例:

```javascript
app.on('ready', function() {
  require('electron').powerMonitor.on('suspend', function() {
    console.log('The system is going to sleep');
  });
});
```

## イベント

`power-monitor`モジュールは次のイベントを出力します。:

### イベント: 'suspend'

システムがサスペンドのときに出力されます。

### イベント: 'resume'

システムがレジュームのときに出力されます。

### イベント: 'on-ac'

システムがACパワーに変わったときに出力されます。

### イベント: 'on-battery'

システムがバッテリーパワーに変わったときに出力されます。

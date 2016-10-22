# powerSaveBlocker

`power-save-blocker` 模組是用來防止系統進入省電模式 low-power (sleep) mode
因此讓應用程式可以保持系統和螢幕的活躍 (active)。

舉例來說:

```javascript
var powerSaveBlocker = require('power-save-blocker')

var id = powerSaveBlocker.start('prevent-display-sleep')
console.log(powerSaveBlocker.isStarted(id))

powerSaveBlocker.stop(id)
```

## 方法 (Methods)

`power-save-blocker` 模組有以下幾個方法:

### `powerSaveBlocker.start(type)`

* `type` String - Power save blocker type.
  * `prevent-app-suspension` - 防止一個應用程式進入睡眠 (suspended)。 將保持系統活躍，
    但允許螢幕被關閉。 使用案例：下載一個檔案或是播放音樂。
  * `prevent-display-sleep`- 防止螢幕進入睡眠。將保持系統和螢幕的活躍。
    使用案例：播放影片

當防止系統進入省電模式 low-power (sleep) mode 。 會回傳一個識別的整數來代表 power save blocker

**注意:** `prevent-display-sleep` 比 `prevent-app-suspension` 擁有較高的優先權。
只有高的優先全力才會有效，換句話說 `prevent-display-sleep` 總是會優先於 `prevent-app-suspension`

例如，一個 API 呼叫 A 請求去做 `prevent-app-suspension`，而另外一個 B 請求去做 `prevent-display-sleep`
 `prevent-display-sleep` 將會被使用，直到 B 停止他的請求，`prevent-app-suspension` 才會被使用。

### `powerSaveBlocker.stop(id)`

* `id` Integer - power save blocker 會回傳 id 透過 `powerSaveBlocker.start`.

將指定的 id 停止 power save blocker

### `powerSaveBlocker.isStarted(id)`

* `id` Integer - power save blocker 會回傳 id 透過 `powerSaveBlocker.start`.

不管對應的 `powerSaveBlocker` 是否已經啟動，將會回傳一個布林值 (boolean)

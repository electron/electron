# Cookie 物件

* `name` 字串 - cookie 的名字.
* `value` 字串 - cookie 的值.
* `domain` 字串 - cookie 的域名.
* `hostOnly` 字串 - cookie 是否為Host-Only.
* `path` 字串 - cookie 的路徑.
* `secure` 布林 - cookie 的網域是否安全 （https).
* `httpOnly` 布林 - cookie 是否能載運行在 HTTP.
* `session` 布林 - cookie 為 短期session 或者 長期cookie，若是長期cookie得包含一個截止日期.
* `expirationDate` 雙精度浮點數 (可選) - cookie 的截止日期，當 `session` 設定為 時可用，值為 UNIX時間.

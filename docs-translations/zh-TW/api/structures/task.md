# Task 物件

* `program` 字串 - 程式執行的路徑，通常你應該指定 `process.execPath`，此值為當前程式執行的路徑。
* `arguments` 字串 - `program` 在命令列下執行的附加參數。
* `title` 字串 - JumpList 內顯示的標題。
* `description` 字串 - 關於此物件的描述。
* `iconPath` 字串 - 此物件的 icon 路徑，被顯示在 JumpList 中，通常你可以指定 `process.execPath`去顯示當前物件的 icon 。
* `iconIndex` 數字 - icon 的索引，如果一個檔案包由兩個或以上的的 icon, 設定此值去指定 icon, 如果檔案內只有一個 icon, 此值為 0 。

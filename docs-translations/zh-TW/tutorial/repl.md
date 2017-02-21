# REPL

讀取（Read）-求值（Evaluate）-輸出（Print）-循環（Loop）（REPL）是一個簡單、
互動式的程式語言環境，藉由簡單的使用者輸入（例如：簡單的表達式），計算輸入，然後輸出結果給使用者。

`repl` 模組實現了 REPL 功能，可以藉由以下的方式存取：

* 假設你已在專案內部安裝了 `electron` 或是 `electron-prebuilt`：

  ```sh
  ./node_modules/.bin/electron --interactive
  ```
* 假設你已全域地安裝了 `electron` 或是 `electron-prebuilt`：

  ```sh
  electron --interactive
  ```

以上的指令只會為主行程建立 REPL。
你可以從開發者工具的主控台分頁來為渲染行程取得一個 REPL。

**註記：** `electron --interactive` 在 Windows 平台上並不適用。

在 [Node.js REPL docs](https://nodejs.org/dist/latest/docs/api/repl.html) 裡可以找到更多資訊。

# Electron約定

:+1::tada: 首先，感謝抽出時間做出貢獻的每一個人! :tada::+1:

該項目遵守貢獻者約定 [code of conduct](CODE_OF_CONDUCT.md)。
我們希望貢獻者能遵守此約定。如果有發現任何不被接受的行為，請回報至electron@github.com(PS:請用英語)。

下在是一些用於改進Electron的指南。
這些只是指導方針，而不是規則，做出你認為最適合的判斷，並隨時
在 pull request 中提出對該文件的更改。

## 提交 Issues

* 你可以在此創建一個 issue [here](https://github.com/electron/electron/issues/new)，
但在此之前，請閱讀以下注意事項，其中應包含盡可能多的細節。
如果可以的話，請包括:
  * 你所使用的 Electron 版本
  * 你所使用的系統
  * 如果適用，請包括:你做了什麼時發生了問題，以及你所預期的結果。
* 其他有助於解決你的 issue 的選項:
  * 截圖和動態GIF
  * 終端機和開發工具中的錯誤訊息或警告。
  * 執行 [cursory search](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)
  檢查是否已存在類似問題

## 提交 Pull Requests

* 可以的話，在 pull request 包含截圖和動態GIF。
* 遵守 JavaScript, C++, and Python [coding style defined in docs](/docs/development/coding-style.md).
* 使用 [Markdown](https://daringfireball.net/projects/markdown) 撰寫文件。
  請參考 [Documentation Styleguide](/docs/styleguide.md).
* 使用簡單明瞭的提交訊息。請參考 [Commit Message Styleguide](#git-commit-messages).

## 文件樣式

### 通用代碼

* 以空行做文件結尾。
* 按照以下順序載入模組:
  * 添加 Node Modules (參考 `path`)
  * 添加 Electron Modules (參考 `ipc`, `app`)
  * 本地模組(使用相對路徑)
* 按照以下的順序排序類別(class)的屬性:
  * 類別的方法和屬性 (方法以 `@` 作為開頭)
  * 實體(Instance)的方法和屬性
* 避免使用平台相依代碼:
  * 使用 `path.join()` 來連接檔案名稱。
  * 當需要使用臨時目錄時，使用 `os.tmpdir()` 而不是 `/tmp` 。
* 在 function 結尾使用明確的 `return` 。
  * 不要使用 `return null`, `return undefined`, `null`, 或 `undefined`

### Git 提交訊息 (鑑於進行Git提交時需要英文書寫，此處暫不翻譯)

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally
* When only changing documentation, include `[ci skip]` in the commit description
* Consider starting the commit message with an applicable emoji:
  * :art: `:art:` when improving the format/structure of the code
  * :racehorse: `:racehorse:` when improving performance
  * :non-potable_water: `:non-potable_water:` when plugging memory leaks
  * :memo: `:memo:` when writing docs
  * :penguin: `:penguin:` when fixing something on Linux
  * :apple: `:apple:` when fixing something on macOS
  * :checkered_flag: `:checkered_flag:` when fixing something on Windows
  * :bug: `:bug:` when fixing a bug
  * :fire: `:fire:` when removing code or files
  * :green_heart: `:green_heart:` when fixing the CI build
  * :white_check_mark: `:white_check_mark:` when adding tests
  * :lock: `:lock:` when dealing with security
  * :arrow_up: `:arrow_up:` when upgrading dependencies
  * :arrow_down: `:arrow_down:` when downgrading dependencies
  * :shirt: `:shirt:` when removing linter warnings

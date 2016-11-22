# 關於 Electron

[Electron](http://electron.atom.io) 是 GitHub 為了透過 HTML, CSS 和 JavaScript 開發跨平台桌面應用程式, 所使用的一個開放原始碼函式庫。為了達成這個目標，Electron 把 [Chromium](https://www.chromium.org/Home) 和 [Node.js](https://nodejs.org) 整合成單一的執行程式，應用程式可以在 Mac, Windows, 和 Linux 作業系統下執行。

Electron 在 2013 年創立，做為 [Atom](https://atom.io) (一款由GitHub 推出，可以快速修改調整的文字編輯器) 的程式框架, 這兩個專案在 2014 年春天開放原始碼。

在這之後，Electron 變成一個非成流行的工具，為許多開放原始碼開發者、新創事業，以及已發展的公司所使用(請見[Apps](http://electron.atom.io/apps/))。

若要了解更多關於 Electron 開發者或發行版的資訊，或想要開始使用 Electron 開發應用程式，可以參考[快速入門](https://github.com/electron/electron/blob/master/docs-translations/zh-TW/tutorial/quick-start.md)

## 核心成員及貢獻者

Electron 由 GitHub 的專案小組，以及社群中[活躍的貢獻者](https://github.com/electron/electron/graphs/contributors) 共同維護。這些維護者之中有些是個人貢獻，有些是使用 Electron 開發應用程式的大型企業。我們非常樂意將頻繁提出貢獻的貢獻者加入維護者的行列。請參考 [contributing to Electron](https://github.com/electron/electron/blob/master/CONTRIBUTING.md).

## 發行

Electron 對於[發行](https://github.com/electron/electron/releases)這件事是非常頻繁的，每當有重大的錯誤修正，新的API，或是 Chromium、Node.js 有重大更新時我們都會提出新的發行。

### 更新相依性

Electron 在 Chromium 提出新的穩定版本時會提出更新，時間通常為一至兩周，由我們在更新時的開發成果決定.

當 Node.js 提出新的發行版時, Electron 為了讓新版本更穩定，通常會等待一個月後才提出 Electron 的新版本。

在 Electron 中, Node.js and Chromium 共用一個 V8 實例--通常是 Chromium 所使用的版本。在許多時候這 _可以運作_，但有時候需要更新 Node.js.


### 版本控制

由於同時高度依賴 Node.js 和 Chromium， Electron 在版本控制上處於一個有點特別的情況，所以不太遵照[ `semver`規範](http://semver.org)。你必須隨時參考一個特定的 Electron 版本。請參考 [Read more about Electron's versioning](http://electron.atom.io/docs/tutorial/electron-versioning/) 或是察看 [versions currently in use](https://electron.atom.io/#electron-versions).

### 長期支援

Electron 目前並未對舊的版本提供長期支援，如果你目前使用的 Electron 版本可以運作, 你可以隨自己的喜好持續使用。如果你想使用新版本所提供的功能，你必須更新到較新的版本。

一個主要的更新是在 `v1.0.0` 版本。如果你使用比這個版本更舊的 Electron，你必須參考 [read more about the `v1.0.0` changes](http://electron.atom.io/blog/2016/05/11/electron-1-0).

## 核心哲學

為了讓 Electron (在檔案大小上) 更小，且能永續經營 (擴展相依性 和 APIs)，Electron 對核心專案涵蓋的範圍會有所限制。

舉例來說, Electron 只使用 Chromium 在圖形渲染上的函式庫，而不使用整個 Chromium，這讓它更容易隨著 Chromium 更新，但也表示有些在 Google Chrome 瀏覽器上擁有的功能，在 Electron中並不存在。

會加入 Electron 的新功能，主要是原生的 APIs。如果某個功能被與它相關的 Node.js 模組所擁有，它在 Electron 中也必須存在。請參考[Electron tools built by the community](http://electron.atom.io/community).

## 歷史

這些是在 Electron 歷史中的重大里程碑.

| :calendar: | :tada: |
| --- | --- |
| **2013年4月**| [Atom Shell 專案開始](https://github.com/electron/electron/commit/6ef8875b1e93787fa9759f602e7880f28e8e6b45).|
| **2014年5月** | [Atom Shell 專案開放原始碼](http://blog.atom.io/2014/05/06/atom-is-now-open-source.html). |
| **2015年4月** | [Atom Shell 專案重新命名為 Electron](https://github.com/electron/electron/pull/1389). |
| **2016年5月** | [Electron 發行 `v1.0.0` 版](http://electron.atom.io/blog/2016/05/11/electron-1-0).|
| **2016年5月** | [Electron 應用程式可以和 Mac App 市集相容](http://electron.atom.io/docs/tutorial/mac-app-store-submission-guide).|
| **2016年8月** | [Windows 市集支援 Electron 應用程式](http://electron.atom.io/docs/tutorial/windows-store-guide).|

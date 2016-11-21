# 可存取性

產生具可存取性的應用程式是非常重要的，我們非常高興能介紹 [Devtron](http://electron.atom.io/devtron) 和 [Spectron](http://electron.atom.io/spectron) 這兩個新功能，這可以讓開發者更有機會能開發對大家來說更棒的應用程式。

---

在 Electron 應用程式中，可存取性的議題和在開發網站時非常類似，因為在根本上兩者都使用 HTML。然而對 Electron 應用程式來說，你不能為了增加可存取性而使用線上的程式審計機制，因為你的應用程式並沒有一個 URL 連結能夠引導審計者.

這些新的功能也為你的 Electron 應用程式帶來新的審計工具，你可以選擇要透過 Spectron 為你的測試增加審計，或是透過 Devtron 在開發者工具中使用。請參考 [accessibility documentation](http://electron.atom.io/docs/tutorial/accessibility) 以獲得更多資訊。

### Spectron

在 Spectron 測試框架中, 你可以對應用程式裡的每個視窗或每個 `<webview>` 標籤做審計。例如:

```javascript
app.client.auditAccessibility().then(function (audit) {
  if (audit.failed) {
    console.error(audit.message)
  }
})
```

要要了解更多有關這個功能的資訊，請參考 [Spectron's documentation](https://github.com/electron/spectron#accessibility-testing).

### Devtron

在 Devtron中, 現在有新的 accessibility 分頁讓你可以審計一個應用程式中的頁面，並對結果做過慮和排序。

![devtron 截圖](https://cloud.githubusercontent.com/assets/1305617/17156618/9f9bcd72-533f-11e6-880d-389115f40a2a.png)

這兩個工具都使用 Google 為 Chrome 所建立的 [Accessibility Developer Tools](https://github.com/GoogleChrome/accessibility-developer-tools) 函式庫。你可以在 [repository's wiki](https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules) 學到更多在這個函式庫中有關可存取性審計的資訊。

如果你知道其他有關 Electron 可存取性來說更好的工具, 請把他們透過 pull request 加入 [accessibility documentation](http://electron.atom.io/docs/tutorial/accessibility) 。

# 更新應用程式

有很多種方式能夠更新 Electron 應用程式，其中最簡單也最正式的一種是使用 [Squirrel](https://github.com/Squirrel) 框架，以及 Electron's [自動更新](../api/auto-updater.md)模組。

## 部署更新伺服器

首先，您需要部署一個伺服器讓[自動更新](../api/auto-updater.md)模組能夠下載更新檔案。

您可以選擇以下幾種服務：

- [Hazel](https://github.com/zeit/hazel) – Simple update server for open-source 
apps. Pulls from 
[GitHub Releases](https://help.github.com/articles/creating-releases/) 
and can be deployed for free on [Now](https://zeit.co/now).
- [Nuts](https://github.com/GitbookIO/nuts) – Also uses 
[GitHub Releases](https://help.github.com/articles/creating-releases/), 
but caches app updates on disk and supports private repositories.
- [electron-release-server](https://github.com/ArekSredzki/electron-release-server) – 
Provides a dashboard for handling releases

如果您的應用程式是使用 [electron-builder][electron-builder-lib] 打包，您可以使用 [electron-updater] 模組，它不需要部署伺服器且允許從靜態網址下載，例如 S3 或 GitHub 。

## 在您的應用程式中實作更新

當您部署好更新伺服器後，在您的程式碼中引用必要的模組，以下的程式碼將以 [Hazel](https://github.com/zeit/hazel) 服務為範例。

**重要：** 請確認以下的程式碼只會在您打包後的應用程式內執行，且不在開發環境中，您可以使用 [electron-is-dev](https://github.com/sindresorhus/electron-is-dev) 來確認當前環境。

```js
const {app, autoUpdater, dialog} = require('electron')
```

接下來，構造更新伺服器網址並傳入 [autoUpdater](../api/auto-updater.md) ：

```js
const server = 'https://your-deployment-url.com'
const feed = `${server}/update/${process.platform}/${app.getVersion()}`

autoUpdater.setFeedURL(feed)
```

最後，檢查更新，以下的範例將會每分鐘檢查一次。

```js
setInterval(() => {
  autoUpdater.checkForUpdates()
}, 60000)
```

當你[打包](../tutorial/application-distribution.md)您的應用程式後，應用程式將會收到每次您在 [GitHub Release](https://help.github.com/articles/creating-releases/) 的更新檔案。

## 應用更新

現在您已為應用程式部署了一個基礎的更新機制，為了要確保所有的用戶都會收到更新通知，您可以使用 autoUpdater API [events](../api/auto-updater.md#events) 來實作：

```js
autoUpdater.on('update-downloaded', (event, releaseNotes, releaseName) => {
  const dialogOpts = {
    type: 'info',
    buttons: ['Restart', 'Later'],
    title: 'Application Update',
    message: process.platform === 'win32' ? releaseNotes : releaseName,
    detail: 'A new version has been downloaded. Restart the application to apply the updates.'
  }

  dialog.showMessageBox(dialogOpts, (response) => {
    if (response === 0) autoUpdater.quitAndInstall()
  })
})
```

同時確認所有的錯誤都有被[處理](../api/auto-updater.md#event-error)，以下將以記錄錯誤至 `stderr` 為範例：

```js
autoUpdater.on('error', message => {
  console.error('There was a problem updating the application')
  console.error(message)
})
```

[electron-builder-lib]: https://github.com/electron-userland/electron-builder
[electron-updater]: https://www.electron.build/auto-update

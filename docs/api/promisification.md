## Promisification

The Electron team is currently undergoing an initiative to convert callback-based functions in Electron to return Promises. During this transition period, both the callback and Promise-based versions of these functions will work correctly, and will both be documented.

To enable deprecation warnings for these updated functions, use the `process.enablePromiseAPI` runtime flag.

When a majority of affected functions are migrated, this flag will be enabled by default and all developers will be able to see these deprecation warnings. At that time, the callback-based versions will also be removed from documentation. This document will be continuously updated as more functions are converted.

### Candidate Functions

- [app.importCertificate(options, callback)](https://github.com/electron/electron/blob/master/docs/api/app.md#importCertificate)
- [contentTracing.getTraceBufferUsage(callback)](https://github.com/electron/electron/blob/master/docs/api/content-tracing.md#getTraceBufferUsage)
- [dialog.showOpenDialog([browserWindow, ]options[, callback])](https://github.com/electron/electron/blob/master/docs/api/dialog.md#showOpenDialog)
- [dialog.showSaveDialog([browserWindow, ]options[, callback])](https://github.com/electron/electron/blob/master/docs/api/dialog.md#showSaveDialog)
- [dialog.showMessageBox([browserWindow, ]options[, callback])](https://github.com/electron/electron/blob/master/docs/api/dialog.md#showMessageBox)
- [dialog.showCertificateTrustDialog([browserWindow, ]options, callback)](https://github.com/electron/electron/blob/master/docs/api/dialog.md#showCertificateTrustDialog)
- [inAppPurchase.purchaseProduct(productID, quantity, callback)](https://github.com/electron/electron/blob/master/docs/api/in-app-purchase.md#purchaseProduct)
- [inAppPurchase.getProducts(productIDs, callback)](https://github.com/electron/electron/blob/master/docs/api/in-app-purchase.md#getProducts)
- [netLog.stopLogging([callback])](https://github.com/electron/electron/blob/master/docs/api/net-log.md#stopLogging)
- [protocol.isProtocolHandled(scheme, callback)](https://github.com/electron/electron/blob/master/docs/api/protocol.md#isProtocolHandled)
- [ses.getCacheSize(callback)](https://github.com/electron/electron/blob/master/docs/api/session.md#getCacheSize)
- [ses.clearCache(callback)](https://github.com/electron/electron/blob/master/docs/api/session.md#clearCache)
- [ses.clearStorageData([options, callback])](https://github.com/electron/electron/blob/master/docs/api/session.md#clearStorageData)
- [ses.setProxy(config, callback)](https://github.com/electron/electron/blob/master/docs/api/session.md#setProxy)
- [ses.resolveProxy(url, callback)](https://github.com/electron/electron/blob/master/docs/api/session.md#resolveProxy)
- [ses.clearHostResolverCache([callback])](https://github.com/electron/electron/blob/master/docs/api/session.md#clearHostResolverCache)
- [ses.getBlobData(identifier, callback)](https://github.com/electron/electron/blob/master/docs/api/session.md#getBlobData)
- [ses.clearAuthCache(options[, callback])](https://github.com/electron/electron/blob/master/docs/api/session.md#clearAuthCache)
- [contents.executeJavaScript(code[, userGesture, callback])](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#executeJavaScript)
- [contents.hasServiceWorker(callback)](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#hasServiceWorker)
- [contents.unregisterServiceWorker(callback)](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#unregisterServiceWorker)
- [contents.print([options], [callback])](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#print)
- [contents.printToPDF(options, callback)](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#printToPDF)
- [contents.savePage(fullPath, saveType, callback)](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#savePage)
- [webFrame.executeJavaScript(code[, userGesture, callback])](https://github.com/electron/electron/blob/master/docs/api/web-frame.md#executeJavaScript)
- [webFrame.executeJavaScriptInIsolatedWorld(worldId, scripts[, userGesture, callback])](https://github.com/electron/electron/blob/master/docs/api/web-frame.md#executeJavaScriptInIsolatedWorld)
- [webviewTag.executeJavaScript(code[, userGesture, callback])](https://github.com/electron/electron/blob/master/docs/api/webview-tag.md#executeJavaScript)
- [webviewTag.printToPDF(options, callback)](https://github.com/electron/electron/blob/master/docs/api/webview-tag.md#printToPDF)

### Converted Functions

- [app.getFileIcon(path[, options], callback)](https://github.com/electron/electron/blob/master/docs/api/app.md#getFileIcon)
- [contents.capturePage([rect, ]callback)](https://github.com/electron/electron/blob/master/docs/api/web-contents.md#capturePage)
- [contentTracing.getCategories(callback)](https://github.com/electron/electron/blob/master/docs/api/content-tracing.md#getCategories)
- [contentTracing.startRecording(options, callback)](https://github.com/electron/electron/blob/master/docs/api/content-tracing.md#startRecording)
- [contentTracing.stopRecording(resultFilePath, callback)](https://github.com/electron/electron/blob/master/docs/api/content-tracing.md#stopRecording)
- [cookies.flushStore(callback)](https://github.com/electron/electron/blob/master/docs/api/cookies.md#flushStore)
- [cookies.get(filter, callback)](https://github.com/electron/electron/blob/master/docs/api/cookies.md#get)
- [cookies.remove(url, name, callback)](https://github.com/electron/electron/blob/master/docs/api/cookies.md#remove)
- [cookies.set(details, callback)](https://github.com/electron/electron/blob/master/docs/api/cookies.md#set)
- [debugger.sendCommand(method[, commandParams, callback])](https://github.com/electron/electron/blob/master/docs/api/debugger.md#sendCommand)
- [desktopCapturer.getSources(options, callback)](https://github.com/electron/electron/blob/master/docs/api/desktop-capturer.md#getSources)
- [protocol.isProtocolHandled(scheme, callback)](https://github.com/electron/electron/blob/master/docs/api/protocol.md#isProtocolHandled)
- [shell.openExternal(url[, options, callback])](https://github.com/electron/electron/blob/master/docs/api/shell.md#openExternal)
- [webviewTag.capturePage([rect, ]callback)](https://github.com/electron/electron/blob/master/docs/api/webview-tag.md#capturePage)
- [win.capturePage([rect, ]callback)](https://github.com/electron/electron/blob/master/docs/api/browser-window.md#capturePage)

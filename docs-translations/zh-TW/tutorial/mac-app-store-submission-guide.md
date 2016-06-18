# Mac App Store 提交指引

自從版本 v0.34.0 開始，Electron 允許提交打包好的應用程式到 Mac App Store(MAS)，這個指引提供了以下資訊：如何提交你的應用程式和 MAS 的建置限制。

__Note:__ 提交一個應用程式到 Mac App Store 需要註冊要付費的 [Apple Developer
Program][developer-program].

## 如何提交你的應用程式

以下步驟介紹了一個簡單的方法提交你的應用程式到 Mac App Store，然而這些步驟不保證你的應用程式會被 Apple 批准，你仍然需要閱讀 Apple 的 [Submitting Your App][submitting-your-app] 指引來達到 Mac App Store 的要求。

### 取得認證

要提交你的應用程式到 Mac App Store，你首先必須取得 Apple 的認證，你可以遵循這些網路上的 [existing guides][nwjs-guide]。

### 簽署你的應用程式

取得了 Apple 的認證以後，你可以遵照 [Application Distribution](application-distribution.md) 來打包你的應用程式，然後進行你應用程式的簽署，這個步驟基本上與其他程式相同，但重點在於你要一一為每個 Electron 的相依套件做簽署。

首先，你需要準備兩個管理權限用的檔案。

`child.plist`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
    <key>com.apple.security.app-sandbox</key>
    <true/>
    <key>com.apple.security.inherit</key>
    <true/>
  </dict>
</plist>
```

`parent.plist`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
    <key>com.apple.security.app-sandbox</key>
    <true/>
  </dict>
</plist>
```

接著遵照下面的腳本簽署你的應用程式：

```bash
#!/bin/bash

# Name of your app.
APP="YourApp"
# The path of you app to sign.
APP_PATH="/path/to/YouApp.app"
# The path to the location you want to put the signed package.
RESULT_PATH="~/Desktop/$APP.pkg"
# The name of certificates you requested.
APP_KEY="3rd Party Mac Developer Application: Company Name (APPIDENTITY)"
INSTALLER_KEY="3rd Party Mac Developer Installer: Company Name (APPIDENTITY)"

FRAMEWORKS_PATH="$APP_PATH/Contents/Frameworks"

codesign --deep -fs "$APP_KEY" --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework/Libraries/libnode.dylib"
codesign --deep -fs "$APP_KEY" --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework/Electron Framework"
codesign --deep -fs "$APP_KEY" --entitlements child.plist "$FRAMEWORKS_PATH/Electron Framework.framework/"
codesign --deep -fs "$APP_KEY" --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper.app/"
codesign --deep -fs "$APP_KEY" --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper EH.app/"
codesign --deep -fs "$APP_KEY" --entitlements child.plist "$FRAMEWORKS_PATH/$APP Helper NP.app/"
codesign  -fs "$APP_KEY" --entitlements parent.plist "$APP_PATH"
productbuild --component "$APP_PATH" /Applications --sign "$INSTALLER_KEY" "$RESULT_PATH"
```

如果你是第一次使用 macOS 下的應用程式沙盒(app sandboxing)，你應該也要閱讀 Apple 的 [Enabling App Sandbox][enable-app-sandbox] 以具備基本概念，然後把你的應用程式會用到的 key 的權限都加入管理權現的檔案中。

### 上傳你的應用程式和提交檢視

當你簽署好你的應用程式後，你可以使用應用程式載入器(Application Loader)把他上傳到 iTunes，處理中請保持連線順暢，在上傳以前請確保你已經 [建立一個紀錄][create-record]，機著你就可以提交你的應用程式去檢視了。

## MAS 建置的限制

為了滿足應用程式沙盒的所有的要求，以下模組需要在 MAS 建置過程中被停用：

* `crash-reporter`
* `auto-updater`

然後以下的動作已經被變更：

* 在某些機器上影像捕捉可能不能運作
* 特定的存取特性可能無法運作
* 應用程式將不管 DNS 的改變

此外，由於使用了應用程式沙盒，那些可以被應用程式存取的資源會被嚴格限制，你可以閱讀 [App Sandboxing][app-sandboxing] 以取得更多資訊。

[developer-program]: https://developer.apple.com/support/compare-memberships/
[submitting-your-app]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/AppDistributionGuide/SubmittingYourApp/SubmittingYourApp.html
[nwjs-guide]: https://github.com/nwjs/nw.js/wiki/Mac-App-Store-%28MAS%29-Submission-Guideline#first-steps
[enable-app-sandbox]: https://developer.apple.com/library/ios/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html
[create-record]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/CreatingiTunesConnectRecord.html
[submit-for-review]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/SubmittingTheApp.html
[app-sandboxing]: https://developer.apple.com/app-sandboxing/

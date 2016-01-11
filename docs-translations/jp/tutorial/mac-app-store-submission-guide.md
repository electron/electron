# Mac App Store 提出ガイド

v0.34.0から、ElectronはMac App Store (MAS)にパッケージ化したアプリを登録することができます。このガイドでは、MASビルド用の制限とアプリを登録する方法についての情報を提供します。

__Note:__ Mac App Storeにアプリを登録するには、費用が発生する[Apple Developer Program][developer-program]に登録する必要があります。

## アプリを登録する方法

次の手順は、Mac App Sotreにアプリを登録する簡単な手順を説明します。しかし、これらの手順はAppleによってAppが承認されることを保証するものではありません。Mac App Storeの要件については、Appleの[Submitting Your App][submitting-your-app]ガイドを読んでおく必要があります。

### 証明書の取得

Mac App StoreにAppを登録するには、最初にAppleから証明書を取得しなければありません。Web上で、[existing guides][nwjs-guide] を参照してください。

### アプリケーションの署名

Appleから証明書を取得した後、[Application Distribution](application-distribution.md)を参照してアプリをパッケージ化し、アプリに証明をします。この手順は、基本的にほかのプログラムと同じですが、Electronが依存するすべてに１つ１つサインします。

最初に、2つの資格ファイルを用意する必要があります。

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

次のスクリプトでアプリをサインします。

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

OS Xで、サンドボックスにアプリを新しく追加した場合、基本的な考え方は、Appleの[Enabling App Sandbox][enable-app-sandbox]を読み、エンタイトルメントファイルにアプリに必要なパーミッションキーを追加します。

### Appをアップロードし、レビューに登録する

アプリに署名後、iTunes ConnectにアップロードするためにApplication Loaderを使用できます。アップロードする前に[created a record][create-record]を確認してください。そして[レビュー用にアプリを登録できます][submit-for-review].

## MAS Buildの制限

アプリのサンドボックスですべての要件を満たすために、MASビルドで次のモジュールを無効にしてください。

* `crash-reporter`
* `auto-updater`

次の挙動を変更してください。

* ビデオキャプチャーはいくつかのマシンで動作しないかもしれません。
* 一部のアクセシビリティ機能が動作しないことがあります。
* アプリはDNSの変更を認識しません。

アプリのサンドボックスでの使用が原因で、アプリがアクセスできるリソースは厳密に制限されています。詳細は、 [App Sandboxing][app-sandboxing] を参照してください。

[developer-program]: https://developer.apple.com/support/compare-memberships/
[submitting-your-app]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/AppDistributionGuide/SubmittingYourApp/SubmittingYourApp.html
[nwjs-guide]: https://github.com/nwjs/nw.js/wiki/Mac-App-Store-%28MAS%29-Submission-Guideline#first-steps
[enable-app-sandbox]: https://developer.apple.com/library/ios/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html
[create-record]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/CreatingiTunesConnectRecord.html
[submit-for-review]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/SubmittingTheApp.html
[app-sandboxing]: https://developer.apple.com/app-sandboxing/

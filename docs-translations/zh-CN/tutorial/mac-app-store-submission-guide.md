# Mac App Store 应用提交向导

自从 v0.34.0，Electron 就允许提交应用包到 Mac App Store
(MAS)。这个向导提供的信息有: 如何提交应用和 MAS 构建的限制。

__注意:__ 提交应用到 Mac App Store 需要参加 [Apple Developer
Program][developer-program]，这需要额外花费。

## 如何提交

下面步骤介绍了一个简单的提交应用到商店方法。然而，这些步骤不能保证你的应用被 Apple 接受；你仍然需要阅读 Apple 的 [Submitting Your App][submitting-your-app] 关于如何满足 Mac App Store 要求的向导。

### 获得证书

为了提交应用到商店，首先需要从 Apple 获得一个证书。可以遵循 [现有向导][nwjs-guide]。

### 软件签名

获得证书之后，你可以使用 [应用部署](application-distribution.md) 打包你的应用，之后进行提交。

首先，你需要在软件包内的 `Info.plist` 中增添一项 `ElectronTeamID`：

```xml
<plist version="1.0">
<dict>
  ...
  <key>ElectronTeamID</key>
  <string>TEAM_ID</string>
</dict>
</plist>
```

之后，你需要准备2个授权文件。

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
    <key>com.apple.security.application-groups</key>
    <string>TEAM_ID.your.bundle.id</string>
  </dict>
</plist>
```

请注意上述 `TEAM_ID` 对应开发者账户的 Team ID，`your.bundle.id` 对应软件打包时使用的 Bundle ID。

然后使用下面的脚本签名你的应用：

```bash
#!/bin/bash

# 应用名称
APP="YourApp"
# 应用路径
APP_PATH="/path/to/YourApp.app"
# 生成安装包路径
RESULT_PATH="~/Desktop/$APP.pkg"
# 开发者应用签名证书
APP_KEY="3rd Party Mac Developer Application: Company Name (APPIDENTITY)"
INSTALLER_KEY="3rd Party Mac Developer Installer: Company Name (APPIDENTITY)"
# 授权文件路径
CHILD_PLIST="/path/to/child.plist"
PARENT_PLIST="/path/to/parent.plist"

FRAMEWORKS_PATH="$APP_PATH/Contents/Frameworks"

codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/Electron Framework.framework/Versions/A/Electron Framework"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/Electron Framework.framework/Versions/A/Libraries/libffmpeg.dylib"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/Electron Framework.framework/Versions/A/Libraries/libnode.dylib"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/Electron Framework.framework"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/$APP Helper.app/Contents/MacOS/$APP Helper"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/$APP Helper.app/"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/$APP Helper EH.app/Contents/MacOS/$APP Helper EH"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/$APP Helper EH.app/"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/$APP Helper NP.app/Contents/MacOS/$APP Helper NP"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$FRAMEWORKS_PATH/$APP Helper NP.app/"
codesign -s "$APP_KEY" -f --entitlements "$CHILD_PLIST" "$APP_PATH/Contents/MacOS/$APP"
codesign -s "$APP_KEY" -f --entitlements "$PARENT_PLIST" "$APP_PATH"

productbuild --component "$APP_PATH" /Applications --sign "$INSTALLER_KEY" "$RESULT_PATH"
```

如果你是 OS X 下的应用沙箱使用新手，应当仔细阅读 Apple 的 [Enabling App Sandbox][enable-app-sandbox] 了解一些基础，然后在授权文件 (entitlements files) 内添加你的应用需要的许可。

### 上传你的应用并检查提交

在签名应用之后，你可以使用 Application Loader 上传软件到 iTunes Connect 进行处理。请确保在上传之前你已经 [创建应用记录][create-record]，再 [提交进行审核][submit-for-review]。

## MAS 构建限制

为了让你的应用满足沙箱的所有条件，在 MAS 构建的时候，下面的模块已被禁用：

* `crashReporter`
* `autoUpdater`

并且下面的行为也改变了:

* 一些视频采集功能无效。
* 某些辅助功能无法访问。
* 应用无法检测 DNS 变化。

也由于应用沙箱的使用方法，应用可以访问的资源被严格限制了；阅读更多信息 [App Sandboxing][app-sandboxing]。

## Electron 使用的加密算法

取决于你所在地方的国家和地区，Mac App Store 或许需要记录你应用的加密算法，甚至要求你提交一个 U.S. 加密注册 (ERN) 许可的复印件。

Electron 使用下列加密算法：

* AES - [NIST SP 800-38A](http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf), [NIST SP 800-38D](http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf), [RFC 3394](http://www.ietf.org/rfc/rfc3394.txt)
* HMAC - [FIPS 198-1](http://csrc.nist.gov/publications/fips/fips198-1/FIPS-198-1_final.pdf)
* ECDSA - ANS X9.62–2005
* ECDH - ANS X9.63–2001
* HKDF - [NIST SP 800-56C](http://csrc.nist.gov/publications/nistpubs/800-56C/SP-800-56C.pdf)
* PBKDF2 - [RFC 2898](https://tools.ietf.org/html/rfc2898)
* RSA - [RFC 3447](http://www.ietf.org/rfc/rfc3447)
* SHA - [FIPS 180-4](http://csrc.nist.gov/publications/fips/fips180-4/fips-180-4.pdf)
* Blowfish - https://www.schneier.com/cryptography/blowfish/
* CAST - [RFC 2144](https://tools.ietf.org/html/rfc2144), [RFC 2612](https://tools.ietf.org/html/rfc2612)
* DES - [FIPS 46-3](http://csrc.nist.gov/publications/fips/fips46-3/fips46-3.pdf)
* DH - [RFC 2631](https://tools.ietf.org/html/rfc2631)
* DSA - [ANSI X9.30](http://webstore.ansi.org/RecordDetail.aspx?sku=ANSI+X9.30-1%3A1997)
* EC - [SEC 1](http://www.secg.org/sec1-v2.pdf)
* IDEA - "On the Design and Security of Block Ciphers" book by X. Lai
* MD2 - [RFC 1319](http://tools.ietf.org/html/rfc1319)
* MD4 - [RFC 6150](https://tools.ietf.org/html/rfc6150)
* MD5 - [RFC 1321](https://tools.ietf.org/html/rfc1321)
* MDC2 - [ISO/IEC 10118-2](https://www.openssl.org/docs/manmaster/crypto/mdc2.html)
* RC2 - [RFC 2268](https://tools.ietf.org/html/rfc2268)
* RC4 - [RFC 4345](https://tools.ietf.org/html/rfc4345)
* RC5 - http://people.csail.mit.edu/rivest/Rivest-rc5rev.pdf
* RIPEMD - [ISO/IEC 10118-3](http://webstore.ansi.org/RecordDetail.aspx?sku=ISO%2FIEC%2010118-3:2004)

如何获取 ERN 许可, 可看这篇文章：[How to legally
submit an app to Apple’s App Store when it uses encryption (or how to obtain an
ERN)][ern-tutorial]。

[developer-program]: https://developer.apple.com/support/compare-memberships/
[submitting-your-app]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/AppDistributionGuide/SubmittingYourApp/SubmittingYourApp.html
[nwjs-guide]: https://github.com/nwjs/nw.js/wiki/Mac-App-Store-%28MAS%29-Submission-Guideline#first-steps
[enable-app-sandbox]: https://developer.apple.com/library/ios/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html
[create-record]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/CreatingiTunesConnectRecord.html
[submit-for-review]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/SubmittingTheApp.html
[app-sandboxing]: https://developer.apple.com/app-sandboxing/
[issue-3871]: https://github.com/electron/electron/issues/3871
[ern-tutorial]: https://carouselapps.com/2015/12/15/legally-submit-app-apples-app-store-uses-encryption-obtain-ern/

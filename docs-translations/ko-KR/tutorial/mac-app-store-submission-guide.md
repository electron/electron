# Mac 앱 스토어 애플리케이션 제출 가이드

Electron은 v0.34.0 버전부터 앱 패키지를 Mac App Store(MAS)에 제출할 수 있게
되었습니다. 이 가이드는 애플리케이션을 앱 스토어에 등록하는 방법과 빌드의 한계에 대한
설명을 제공합니다.

**참고:** Mac App Store에 애플리케이션을 등록하려면
[Apple Developer Program][developer-program]에 등록되어 있어야 하며 비용이 발생할
수 있습니다.

## 앱 스토어에 애플리케이션을 등록하는 방법

다음 몇 가지 간단한 절차에 따라 앱 스토어에 애플리케이션을 등록하는 방법을 알아봅니다.
한가지, 이 절차는 제출한 앱이 Apple로부터 승인되는 것을 보장하지 않습니다. 따라서
여전히 Apple의 [Submitting Your App][submitting-your-app] 가이드를 숙지하고 있어야
하며 앱 스토어 제출 요구 사항을 확실히 인지하고 있어야 합니다.

### 인증서 취득

앱 스토어에 애플리케이션을 제출하려면, 먼저 Apple로부터 인증서를 취득해야 합니다. 취득
방법은 웹에서 찾아볼 수 있는 [가이드][nwjs-guide]를 참고하면 됩니다.

### Team ID 얻기

애플리케이션에 서명하기 전에, 먼저 개발 계정의 Team ID를 알아야 합니다. Team ID를
알아보려면 [Apple Developer Center](https://developer.apple.com/account/)에
로그인한 후, 사이드바의 Membership을 클릭합니다. Team ID는 Membership Information
섹션의 팀 이름 밑에 위치합니다.

### 앱에 서명하기

준비 작업이 끝난 후, [애플리케이션 배포](application-distribution.md) 문서에 따라
애플리케이션을 패키징한 후 애플리케이션에 서명합니다.

먼저, Team ID를 키로 가지고 있는 애플리케이션의 `Info.plist`에 `ElectronTeamID` 키를
추가해야 합니다:

```xml
<plist version="1.0">
<dict>
  ...
  <key>ElectronTeamID</key>
  <string>TEAM_ID</string>
</dict>
</plist>
```

그리고, 다음 두 자격 파일을 준비해야 합니다.

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

`TEAM_ID` 부분은 Team ID로 치환하고, `your.bundle.id` 부분은 애플리케이션의 Bundle
ID로 치환해야 합니다.

그리고 다음 스크립트를 통해 애플리케이션에 서명합니다:

```bash
#!/bin/bash

# 애플리케이션의 이름.
APP="YourApp"
# 서명할 애플리케이션의 경로.
APP_PATH="/path/to/YourApp.app"
# 서명된 패키지의 출력 경로.
RESULT_PATH="~/Desktop/$APP.pkg"
# 요청한 인증서의 이름.
APP_KEY="3rd Party Mac Developer Application: Company Name (APPIDENTITY)"
INSTALLER_KEY="3rd Party Mac Developer Installer: Company Name (APPIDENTITY)"
# plist 파일의 경로.
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

만약 macOS의 샌드박스 개념에 대해 처음 접한다면 Apple의 [Enabling App Sandbox][enable-app-sandbox]
문서를 참고하여 기본적인 개념을 이해해야 합니다. 그리고 자격(plist) 파일에
애플리케이션에서 요구하는 권한의 키를 추가합니다.

그 외에 별로도 [electron-osx-sign][electron-osx-sign] 모듈을 사용하여 직접 서명할
수도 있습니다.

#### 네이티브 모듈에 서명하기

앱 내부에서 사용한 네이티브 모듈도 서명이 필요합니다. `electron-osx-sign`을
사용한다면, 인수 목록에 빌트인 바이너리 경로가 포함되어 있는지 확인해야 합니다:

```bash
electron-osx-sign YourApp.app YourApp.app/Contents/Resources/app/node_modules/nativemodule/build/release/nativemodule
```

참고로 네이티브 모듈이 의도하지 않게 중간 파일을 포함하는 경우도 있으며 이 파일은
포함되어선 안됩니다. (해당 파일에도 서명해야 할 수도 있습니다)
버전 8.1.0 전의 [electron-packager][electron-packager] 는, `--ignore=.+\.o$`
를 추가하여 이 파일을 무시할 수 있습니다. 8.1.0 버전 이후에는 기본적으로
무시됩니다.

### 애플리케이션 업로드

애플리케이션 서명을 완료한 후 iTunes Connect에 업로드하기 위해 Application Loader를
사용할 수 있습니다. 참고로 업로드하기 전에 [레코드][create-record]를 만들었는지
확인해야 합니다.

### 애플리케이션을 심사에 제출

위 과정을 마치면 [애플리케이션을 심사를 위해 제출][submit-for-review]할 수 있습니다.

## MAS 빌드의 한계

모든 애플리케이션 샌드박스에 대한 요구 사항을 충족시키기 위해, 다음 모듈들은 MAS
빌드에서 비활성화됩니다:

* `crashReporter`
* `autoUpdater`

그리고 다음 동작으로 대체됩니다:

* 비디오 캡쳐 기능은 몇몇 장치에서 작동하지 않을 수 있습니다.
* 특정 접근성 기능이 작동하지 않을 수 있습니다.
* 애플리케이션이 DNS의 변경을 감지하지 못할 수 있습니다.
* 로그인할 때 앱을 시작하기 위한 API 는 비활성화 되어있습니다. 다음 문서를 보세요.
https://github.com/electron/electron/issues/7312#issuecomment-249479237

또한 애플리케이션 샌드박스 개념으로 인해 애플리케이션에서 접근할 수 있는 리소스는
엄격하게 제한되어 있습니다. 자세한 내용은 [앱 샌드박싱][app-sandboxing] 문서를
참고하세요.

### 추가적인 권한

Mac 앱 스토어 빌드를 위해 앱에서 사용하는 Electron API에 따라 `parent.plist` 파일에
추가적인 기능에 대한 권한을 추가해야 할 수도 있습니다.

#### 네트워크 접근

서버와 연결하기 위한 외부로 나가는 네트워크 연결 허용 활성화:

```xml
<key>com.apple.security.network.client</key>
<true/>
```

네트워크 리스닝 소켓을 열기 위한 내부로 들어오는 네트워크 연결 허용 활성화:

```xml
<key>com.apple.security.network.server</key>
<true/>
```

자세한 내용은 [네트워크 접근 활성화 문서][network-access] 를 참고하세요.

#### dialog.showOpenDialog

```xml
<key>com.apple.security.files.user-selected.read-only</key>
<true/>
```

자세한 내용은 [User-Selected 파일 접근 활성화 문서][user-selected]를 참고하세요.

#### dialog.showSaveDialog

```xml
<key>com.apple.security.files.user-selected.read-write</key>
<true/>
```

자세한 내용은 [User-Selected 파일 접근 활성화 문서][user-selected]를 참고하세요.

## Electron에서 사용하는 암호화 알고리즘

국가와 살고 있는 지역에 따라, 맥 앱스토어는 제출한 애플리케이션에서 사용하는 암호화
알고리즘의 문서를 요구할 수 있습니다. 심지어 U.S. Encryption Registration (ERN)의
승인 사본을 제출하라고 할 수도 있습니다.

Electron은 다음과 같은 암호화 알고리즘을 사용합니다:

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

ERN의 승인을 얻는 방법은, 다음 글을 참고하는 것이 좋습니다:
[애플리케이션이 암호화를 사용할 때, 합법적으로 Apple의 앱 스토어에 제출하는 방법 (또는 ERN의 승인을 얻는 방법)][ern-tutorial].

**역자주:** [Mac 앱 배포 가이드 공식 한국어 문서](https://developer.apple.com/osx/distribution/kr/)

[developer-program]: https://developer.apple.com/support/compare-memberships/
[submitting-your-app]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/AppDistributionGuide/SubmittingYourApp/SubmittingYourApp.html
[nwjs-guide]: https://github.com/nwjs/nw.js/wiki/Mac-App-Store-%28MAS%29-Submission-Guideline#first-steps
[enable-app-sandbox]: https://developer.apple.com/library/ios/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html
[create-record]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/CreatingiTunesConnectRecord.html
[electron-osx-sign]: https://github.com/electron-userland/electron-osx-sign
[electron-packager]: https://github.com/electron-userland/electron-packager
[submit-for-review]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/SubmittingTheApp.html
[app-sandboxing]: https://developer.apple.com/app-sandboxing/
[ern-tutorial]: https://carouselapps.com/2015/12/15/legally-submit-app-apples-app-store-uses-encryption-obtain-ern/
[temporary-exception]: https://developer.apple.com/library/mac/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/AppSandboxTemporaryExceptionEntitlements.html
[user-selected]: https://developer.apple.com/library/mac/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html#//apple_ref/doc/uid/TP40011195-CH4-SW6]

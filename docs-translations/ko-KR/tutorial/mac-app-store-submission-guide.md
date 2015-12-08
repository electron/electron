# Mac 앱 스토어 어플리케이션 제출 가이드

Electron은 v0.34.0 버전부터 앱 패키지를 Mac App Store(MAS)에 제출할 수 있게
되었습니다. 이 가이드는 어플리케이션을 앱 스토어에 등록하는 방법과 빌드의 한계에 대한
설명을 제공합니다.

__참고:__ Mac App Store에 어플리케이션을 등록하려면
[Apple Developer Program][developer-program]에 등록되어 있어야 하며 비용이 발생할
수 있습니다.

## 앱 스토어에 어플리케이션을 등록하는 방법

다음 몇 가지 간단한 절차에 따라 앱 스토어에 어플리케이션을 등록하는 방법을 알아봅니다.
한가지, 이 절차는 제출한 앱이 Apple로부터 승인되는 것을 보장하지 않습니다. 따라서
여전히 Apple의 [Submitting Your App][submitting-your-app] 가이드를 숙지하고 있어야
하며 앱 스토어 제출 요구 사항을 확실히 인지하고 있어야합니다.

### 인증서 취득

앱 스토어에 어플리케이션을 제출하려면, 먼저 Apple로부터 인증서를 취득해야 합니다. 취득
방법은 웹에서 찾아볼 수 있는 [가이드][nwjs-guide]를 참고하면 됩니다.

### 앱에 서명하기

Apple로부터 인증서를 취득했다면, [어플리케이션 배포](application-distribution.md)
문서에 따라 어플리케이션을 패키징한 후 어플리케이션에 서명 합니다. 이 절차는 기본적으로
다른 프로그램과 같습니다. 하지만 키는 Electron 종속성 파일에 각각 따로 서명 해야 합니다.

첫번째, 다음 두 자격(plist) 파일을 준비합니다.

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

그리고 다음 스크립트에 따라 어플리케이션에 서명합니다:

```bash
#!/bin/bash

# 어플리케이션의 이름
APP="YourApp"
# 서명할 어플리케이션의 경로
APP_PATH="/path/to/YouApp.app"
# 서명된 패키지의 출력 경로
RESULT_PATH="~/Desktop/$APP.pkg"
# 요청한 인증서의 이름
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

만약 OS X의 샌드박스 개념에 대해 처음 접한다면 Apple의 [Enabling App Sandbox][enable-app-sandbox]
문서를 참고하여 기본적인 개념을 이해해야 합니다. 그리고 자격(plist) 파일에
어플리케이션에서 요구하는 권한의 키를 추가합니다.

### 어플리케이션을 업로드하고 심사용 앱으로 제출

어플리케이션 서명을 완료한 후 iTunes Connect에 업로드하기 위해 Application Loader를
사용할 수 있습니다. 참고로 업로드하기 전에 [레코드][create-record]를 만들었는지
확인해야 합니다. 그리고 [심사를 위해 앱을 제출][submit-for-review]할 수 있습니다.

## MAS 빌드의 한계

모든 어플리케이션 샌드박스에 대한 요구 사항을 충족시키기 위해, 다음 모듈들은 MAS
빌드에서 비활성화됩니다:

* `crash-reporter`
* `auto-updater`

그리고 다음 동작으로 대체됩니다:

* 비디오 캡쳐 기능은 몇몇 장치에서 작동하지 않을 수 있습니다.
* 특정 접근성 기능이 작동하지 않을 수 있습니다.
* 어플리케이션이 DNS의 변경을 감지하지 못할 수 있습니다.

또한 어플리케이션 샌드박스 개념으로 인해 어플리케이션에서 접근할 수 있는 리소스는
엄격하게 제한되어 있습니다. 자세한 내용은 [App Sandboxing][app-sandboxing] 문서를
참고하세요.

**역주:** [Mac 앱 배포 가이드 공식 문서](https://developer.apple.com/osx/distribution/kr/)

[developer-program]: https://developer.apple.com/support/compare-memberships/
[submitting-your-app]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/AppDistributionGuide/SubmittingYourApp/SubmittingYourApp.html
[nwjs-guide]: https://github.com/nwjs/nw.js/wiki/Mac-App-Store-%28MAS%29-Submission-Guideline#first-steps
[enable-app-sandbox]: https://developer.apple.com/library/ios/documentation/Miscellaneous/Reference/EntitlementKeyReference/Chapters/EnablingAppSandbox.html
[create-record]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/CreatingiTunesConnectRecord.html
[submit-for-review]: https://developer.apple.com/library/ios/documentation/LanguagesUtilities/Conceptual/iTunesConnect_Guide/Chapters/SubmittingTheApp.html
[app-sandboxing]: https://developer.apple.com/app-sandboxing/

# crashReporter

> 원격 서버에 크래시 정보를 보고합니다.

다음 예시는 윈격 서버에 애플리케이션 크래시 정보를 자동으로 보고하는 예시입니다:

```javascript
const {crashReporter} = require('electron')

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https://your-domain.com/url-to-submit',
  autoSubmit: true
})
```

서버가 크래시 리포트를 받을 수 있도록 설정하기 위해 다음과 같은 프로젝트를 사용할 수도
있습니다:

* [socorro](https://github.com/mozilla/socorro)
* [mini-breakpad-server](https://github.com/electron/mini-breakpad-server)

## Methods

`crash-reporter` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `crashReporter.start(options)`

* `options` Object
  * `companyName` String
  * `submitURL` String - 크래시 리포트는 POST 방식으로 이 URL로 전송됩니다.
  * `productName` String (optional) - 기본값은 `Electron` 입니다.
  * `autoSubmit` Boolean - 유저의 승인 없이 자동으로 오류를 보고합니다. 기본값은
    `true` 입니다.
  * `ignoreSystemCrashHandler` Boolean - 기본값은 `false` 입니다.
  * `extra` Object - 크래시 리포트 시 같이 보낼 추가 정보를 지정하는 객체입니다.
    문자열로 된 속성만 정상적으로 보내집니다. 중첩된 객체는 지원되지 않습니다.

다른 crashReporter API를 사용하기 전에 이 메서드를 먼저 호출해야 합니다.

**참고:** macOS에선 Windows와 Linux의 `breakpad`와 달리 새로운 `crashpad`
클라이언트를 사용합니다. 오류 수집 기능을 활성화 시키려면 오류를 수집하고 싶은 메인
프로세스나 렌더러 프로세스에서 `crashReporter.start` 메서드를 호출하여 `crashpad`를
초기화해야 합니다.

### `crashReporter.getLastCrashReport()`

Returns `Object`:
* `date` String
* `ID` Integer

마지막 크래시 리포트의 날짜와 ID를 반환합니다.
이전 크래시 리포트가 없거나 Crash Reporter가 시작되지 않았을 경우 `null`이 반환됩니다.

### `crashReporter.getUploadedReports()`

Returns `Object[]`:
* `date` String
* `ID` Integer

모든 업로드된 크래시 리포트를 반환합니다. 각 보고는 날짜와 업로드 ID를 포함하고 있습니다.

## crash-reporter 업로드 형식

Crash Reporter는 다음과 같은 데이터를 `submitURL`에 `multipart/form-data` `POST` 방식으로 전송합니다:

* `ver` String - Electron의 버전.
* `platform` String - e.g. 'win32'.
* `process_type` String - e.g. 'renderer'.
* `guid` String - e.g. '5e1286fc-da97-479e-918b-6bfb0c3d1c72'.
* `_version` String - `package.json`내의 `version` 필드.
* `_productName` String - Crash Reporter의 `options` 객체에서 정의한 제품명.
* `prod` String - 기본 제품의 이름. 이 경우 Electron으로 표시됩니다.
* `_companyName` String - Crash Reporter의 `options` 객체에서 정의한 회사명.
* `upload_file_minidump` File - `minidump` 형식의 크래시 리포트 파일.
* Crash Reporter의 `options` 객체에서 정의한 `extra` 객체의 속성들.

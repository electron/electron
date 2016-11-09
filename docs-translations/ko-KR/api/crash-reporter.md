# crashReporter

> 원격 서버에 오류 보고를 제출합니다.

프로세스: [메인](../tutorial/quick-start.md#main-process), [렌더러](../tutorial/quick-start.md#renderer-process)

다음은 윈격 서버에 애플리케이션 오류 보고를 자동으로 제출하는 예시입니다:

```javascript
const {crashReporter} = require('electron')

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https://your-domain.com/url-to-submit',
  autoSubmit: true
})
```

서버가 오류 보고를 허용하고 처리할 수 있게 설정하기 위해, 다음 프로젝트를 사용할
수 있습니다:

* [socorro](https://github.com/mozilla/socorro)
* [mini-breakpad-server](https://github.com/electron/mini-breakpad-server)

오류 보고서는 애플리케이션에 명시된 로컬 임시 디렉토리 폴더에 저장됩니다.
`YourName` 의 `productName` 의 경우, 오류 보고서는 임시 디렉토리 내의
`YourName Crashes` 폴더에 저장됩니다. 오류 보고자를 시작하기전에
`app.setPath('temp', '/my/custom/temp')` API 를 호출하여 이 임시 디렉토리 경로를
정의할 수 있습니다.

## Methods

`crash-reporter` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `crashReporter.start(options)`

* `options` Object
  * `companyName` String
  * `submitURL` String - 오류 보고는 POST 방식으로 이 URL로 전송됩니다.
  * `productName` String (optional) - 기본값은 `Electron` 입니다.
  * `autoSubmit` Boolean - 사용자의 승인 없이 자동으로 오류를 보고합니다.
    기본값은 `true` 입니다.
  * `ignoreSystemCrashHandler` Boolean - 기본값은 `false` 입니다.
  * `extra` Object - 보고서와 함께 보낼 추가 정보를 지정하는 객체입니다.
    문자열로 된 속성만 정상적으로 보내집니다. 중첩된 객체는 지원되지 않습니다.

다른 crashReporter API를 사용하기 전에 이 메서드를 먼저 호출해야 합니다.

**참고:** macOS에선 Windows와 Linux의 `breakpad`와 달리 새로운 `crashpad`
클라이언트를 사용합니다. 오류 수집 기능을 활성화 시키려면 오류를 수집하고 싶은
메인 프로세스나 렌더러 프로세스에서 `crashReporter.start` 메서드를 호출하여
`crashpad` 를 초기화해야 합니다.

### `crashReporter.getLastCrashReport()`

Returns `Object`:

* `date` String
* `ID` Integer

마지막 오류 보고의 날짜와 ID를 반환합니다. 이전 오류 보고가 없거나 오류 보고자가
시작되지 않았을 경우 `null`이 반환됩니다.

### `crashReporter.getUploadedReports()`

Returns `Object[]`:

* `date` String
* `ID` Integer

모든 업로드된 오류 보고를 반환합니다. 각 보고는 날짜와 업로드 ID를 포함하고
있습니다.

## crash-reporter 업로드 형식

오류 보고자는 다음과 같은 데이터를 `submitURL `에 `multipart/form-data` `POST`
방식으로 전송합니다:

* `ver` String - Electron 의 버전.
* `platform` String - 예) 'win32'.
* `process_type` String - 예) 'renderer'.
* `guid` String - 예) '5e1286fc-da97-479e-918b-6bfb0c3d1c72'.
* `_version` String - `package.json` 내의 `version` 필드.
* `_productName` String - `crashReporter` `options` 객체의 제품명.
* `prod` String - 기본 제품의 이름. 이 경우에는 Electron.
* `_companyName` String - `crashReporter` `options` 객체의 회사명.
* `upload_file_minidump` File - `minidump` 형식의 옲 보고.
* `crachReporter` `options` 객체내의 `extra` 객체의 모든 레벨1 속성들.

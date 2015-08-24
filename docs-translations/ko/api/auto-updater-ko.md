# auto-updater

**이 모듈은 현재 OS X에서만 사용할 수 있습니다.**

Windows 어플리케이션 인스톨러를 생성하려면 [atom/grunt-electron-installer](https://github.com/atom/grunt-electron-installer)를 참고하세요.

`auto-updater` 모듈은 [Squirrel.Mac](https://github.com/Squirrel/Squirrel.Mac) 프레임워크의 간단한 Wrapper입니다.

Squirrel.Mac은 업데이트 설치를 위해 `.app` 폴더에
[codesign](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/codesign.1.html)
툴을 사용한 서명을 요구합니다.

## Squirrel

Squirrel은 어플리케이션이 **안전하고 투명한 업데이트**를 제공할 수 있도록 하는데 초점이 맞춰진 OS X 프레임워크입니다.

Squirrel은 사용자에게 어플리케이션의 업데이트를 알릴 필요 없이 서버가 지시하는 버전을 받아온 후 자동으로 업데이트합니다.
이 기능을 사용하면 Squirrel을 통해 클라이언트의 어플리케이션을 지능적으로 업데이트 할 수 있습니다.

요청시 커스텀 헤더 또는 요청 본문에 인증 정보를 포함시킬 수도 있습니다.
서버에선 이러한 요청을 분류 처리하여 적당한 업데이트를 제공할 수 있습니다.

Squirrel JSON 업데이트 요청시 처리는 반드시 어떤 업데이트가 필요한지 요청의 기준에 맞춰 동적으로 생성되어야 합니다.
Squirrel은 사용해야 하는 업데이트 선택하는 과정을 서버에 의존합니다. [서버 지원](#server-support)을 참고하세요.

Squirrel의 인스톨러는 오류에 관대하게 설계되었습니다. 그리고 업데이트가 유효한지 확인합니다.

## 업데이트 요청

Squirrel은 업데이트 확인을 위해 클라이언트 어플리케이션의 요청은 무시합니다.
Squirrel은 응답을 분석해야 할 책임이 있기 때문에 `Accept: application/json`이 요청 헤더에 추가됩니다.

업데이트 응답과 본문 포맷에 대한 요구 사항은 [Server Support](#server-support)를 참고하세요.

업데이트 요청에는 서버가 해당 어플리케이션이 어떤 버전을 사용해야 하는지 판단하기 위해 *반드시* 버전 식별자를 포함시켜야 합니다.
추가로 OS 버전, 사용자 이름 같은 다른 식별 기준을 포함하여 서버에서 적합한 어플리케이션을 제공할 수 있도록 할 수 있습니다.

버전 식별자와 다른 기준을 특정하는 업데이트 요청 폼을 서버로 전달하기 위한 공통적인 방법으로 쿼리 인자를 사용하는 방법이 있습니다:

```javascript
// On the main process
var app = require('app');
var autoUpdater = require('auto-updater');
autoUpdater.setFeedUrl('http://mycompany.com/myapp/latest?version=' + app.getVersion());
```

## 서버 지원

업데이트를 제공하는 서버는 반드시 클라이언트로부터 받은 [Update Request](#update-requests)를 기반으로 업데이트를 처리할 수 있어야 합니다.

만약 업데이트 요청이 들어오면 서버는 반드시 [200 OK](http://tools.ietf.org/html/rfc2616#section-10.2.1) 상태 코드를 포함한
[업데이트 JSON](#update-json-format)을 본문으로 보내야 합니다.
이 응답을 받으면 Squirrel은 이 업데이트를 다운로드할 것입니다. 참고로 현재 설치된 버전과 서버에서 받아온 새로운 버전이 같아도 상관하지 않고 무조건 받습니다.
업데이트시 버전 중복을 피하려면 서버에서 클라이언트 업데이트 요청에 대해 통보하지 않으면 됩니다.

만약 따로 업데이트가 없다면 [204 No Content](http://tools.ietf.org/html/rfc2616#section-10.2.5) 상태 코드를 반환해야 합니다.
Squirrel은 지정한 시간이 지난 후 다시 업데이트를 확인합니다.

## JSON 포맷 업데이트

업데이트가 사용 가능한 경우 Squirrel은 다음과 같은 구조의 json 데이터를 응답으로 받습니다:

```json
{
  "url": "http://mycompany.com/myapp/releases/myrelease",
  "name": "My Release Name",
  "notes": "Theses are some release notes innit",
  "pub_date": "2013-09-18T12:29:53+01:00"
}
```

응답 json 데이터에서 "url" 키는 필수적으로 포함해야 하고 다른 키들은 옵션입니다.

Squirrel은 "url"로 `Accept: application/zip` 헤더와 함께 업데이트 zip 파일을 요청합니다.
향후 업데이트 포맷에 대해 서버에서 적절한 포맷을 반환할 수 있도록 MIME 타입을 `Accept` 헤더에 담아 요청합니다.

`pub_date`은 ISO 8601 표준에 따라 포맷된 날짜입니다.

## Event: error

* `event` Event
* `message` String

업데이트시 에러가 나면 발생하는 이벤트입니다.

## Event: checking-for-update

업데이트를 확인하기 시작할 때 발생하는 이벤트입니다.

## Event: update-available

사용 가능한 업데이트가 있을 때 발생하는 이벤트입니다. 이벤트는 자동으로 다운로드 됩니다.

## Event: update-not-available

사용 가능한 업데이트가 없을 때 발생하는 이벤트입니다.

## Event: update-downloaded

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateUrl` String
* `quitAndUpdate` Function

업데이트의 다운로드가 완료되었을 때 발생하는 이벤트입니다. `quitAndUpdate()`를 호출하면 어플리케이션을 종료하고 업데이트를 설치합니다.

## autoUpdater.setFeedUrl(url)

* `url` String

`url`을 설정하고 자동 업데이터를 초기화합니다. `url`은 한번 설정되면 변경할 수 없습니다.

## autoUpdater.checkForUpdates()

서버에 새로운 업데이트가 있는지 요청을 보내 확인합니다. API를 사용하기 전에 `setFeedUrl`를 호출해야 합니다.

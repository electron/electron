# auto-updater

**이 모듈은 현재 OS X에서만 사용할 수 있습니다.**

Windows 어플리케이션 인스톨러를 생성하려면 [atom/grunt-electron-installer](https://github.com/atom/grunt-electron-installer)를 참고하세요.

`auto-updater` 모듈은 [Squirrel.Mac](https://github.com/Squirrel/Squirrel.Mac) 프레임워크의 간단한 Wrapper입니다.

Squirrel.Mac은 업데이트 설치를 위해 `.app` 폴더에
[codesign](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/codesign.1.html)
툴을 사용한 서명을 요구합니다.

## Squirrel

Squirrel은 어플리케이션이 **안전하고 투명한 웹사이트 업데이트**를 할 수 있도록 하는데 초점이 맞춰진 OS X 프레임워크입니다.

Squirrel은 사용자에게 어플리케이션의 업데이트를 알릴 필요 없이 서버가 지시하는 버전을 받아온 후 자동으로 업데이트합니다.
이 기능을 사용하면 Squirrel을 통해 클라이언트의 어플리케이션을 지능적으로 업데이트 할 수 있습니다.

요청시 커스텀 헤더 또는 요청 본문에 인증 정보를 포함시킬 수도 있습니다.
서버에선 이러한 요청을 분류 처리하여 적당한 업데이트를 제공할 수 있습니다.

Squirrel JSON 업데이트 요청시 처리는 반드시 어떤 업데이트가 필요한지 요청의 기준에 맞춰 동적으로 생성되어야 합니다.
Squirrel은 사용해야 하는 업데이트 선택하는 과정을 서버에 의존합니다. [서버 지원](#server-support)을 참고하세요.

Squirrel의 인스톨러는 오류에 관대하게 설계되었습니다. 그리고 업데이트가 유효한지 확인합니다.

## 업데이트 요청

Squirrel is indifferent to the request the client application provides for
update checking. `Accept: application/json` is added to the request headers
because Squirrel is responsible for parsing the response.

For the requirements imposed on the responses and the body format of an update
response see [Server Support](#server-support).

Your update request must *at least* include a version identifier so that the
server can determine whether an update for this specific version is required. It
may also include other identifying criteria such as operating system version or
username, to allow the server to deliver as fine grained an update as you
would like.

How you include the version identifier or other criteria is specific to the
server that you are requesting updates from. A common approach is to use query
parameters, like this:

```javascript
// On the main process
var app = require('app');
var autoUpdater = require('auto-updater');
autoUpdater.setFeedUrl('http://mycompany.com/myapp/latest?version=' + app.getVersion());
```

## 서버 지원

Your server should determine whether an update is required based on the
[Update Request](#update-requests) your client issues.

If an update is required your server should respond with a status code of
[200 OK](http://tools.ietf.org/html/rfc2616#section-10.2.1) and include the
[update JSON](#update-json-format) in the body. Squirrel **will** download and
install this update, even if the version of the update is the same as the
currently running version. To save redundantly downloading the same version
multiple times your server must not inform the client to update.

If no update is required your server must respond with a status code of
[204 No Content](http://tools.ietf.org/html/rfc2616#section-10.2.5). Squirrel
will check for an update again at the interval you specify.

## JSON 포맷 업데이트

When an update is available, Squirrel expects the following schema in response
to the update request provided:

```json
{
  "url": "http://mycompany.com/myapp/releases/myrelease",
  "name": "My Release Name",
  "notes": "Theses are some release notes innit",
  "pub_date": "2013-09-18T12:29:53+01:00",
}
```

The only required key is "url", the others are optional.

Squirrel will request "url" with `Accept: application/zip` and only supports
installing ZIP updates. If future update formats are supported their MIME type
will be added to the `Accept` header so that your server can return the
appropriate format.

`pub_date` if present must be formatted according to ISO 8601.

## Event: error

* `event` Event
* `message` String

Emitted when there is an error updating.

## Event: checking-for-update

Emitted when checking for update has started.

## Event: update-available

Emitted when there is an available update, the update would be downloaded
automatically.

## Event: update-not-available

Emitted when there is no available update.

## Event: update-downloaded

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateUrl` String
* `quitAndUpdate` Function

Emitted when update has been downloaded, calling `quitAndUpdate()` would restart
the application and install the update.

## autoUpdater.setFeedUrl(url)

* `url` String

Set the `url` and initialize the auto updater. The `url` could not be changed
once it is set.

## autoUpdater.checkForUpdates()

Ask the server whether there is an update, you have to call `setFeedUrl` before
using this API.

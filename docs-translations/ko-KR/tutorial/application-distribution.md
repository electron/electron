# 어플리케이션 배포

Electron 어플리케이션을 배포하는 방법은 간단합니다.

먼저 폴더 이름을 `app`로 지정한 후 Electron 리소스 디렉터리에 폴더를 통째로 집어넣기만 하면 됩니다.
리소스 디렉터리는 OS X: `Electron.app/Contents/Resources/` Windows와 Linux: `resources/` 입니다.

예제:

OS X의 경우:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

Windows 와 Linux의 경우:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

그리고 `Electron.app`을 실행하면(Linux에선 `electron` Windows에선 `electron.exe`입니다) Electron 앱이 실행시킵니다.
최종 사용자에겐 이 `electron` 폴더(Electron.app)를 배포하면 됩니다.

## asar로 앱 패키징 하기

소스파일 전체를 복사해서 배포하는 것과는 별개로 [asar](https://github.com/atom/asar) 아카이브를 통해
어플리케이션의 소스코드가 사용자에게 노출되는 것을 방지할 수 있습니다.

`asar` 아카이브를 사용할 땐 단순히 `app` 폴더 대신에 어플리케이션을 패키징한 `app.asar` 파일로 대체하면됩니다.
Electron은 자동으로 `app`폴더 대신 asar 아카이브를 기반으로 어플리케이션을 실행합니다.

OS X의 경우:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

Windows 와 Linux의 경우:

```text
electron/resources/
└── app.asar
```

자세한 내용은 [어플리케이션 패키징](application-packaging.md)에서 찾아볼 수 있습니다.

## 다운로드한 바이너리의 리소스를 앱에 맞게 수정하기

어플리케이션을 Electron에 번들링한 후 해당 어플리케이션에 맞게 리브랜딩 할 수 있습니다.

### Windows

`electron.exe`을 원하는 이름으로 변경할 수 있습니다.
그리고 [rcedit](https://github.com/atom/rcedit) 또는 [ResEdit](http://www.resedit.net)를 사용하여 아이콘을 변경할 수 있습니다.

### OS X

`Electron.app`을 원하는 이름으로 변경할 수 있습니다. 그리고 다음 표시된 어플리케이션 내부 파일에서 
`CFBundleDisplayName`, `CFBundleIdentifier` and `CFBundleName` 필드를 원하는 이름으로 변경해야합니다:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

또한 helper 앱이 프로세스 모니터에 `Electron Helper`로 나오지 않도록 이름을 변경할 수 있습니다.
하지만 반드시 내부 및 모든 helper 앱의 이름을 변경해야 합니다.

어플리케이션 아이콘은 `Electron.app/Contents/Resources/atom.icns`을 원하는 아이콘으로 변경하면 됩니다.

원하는 이름으로 바꾼 어플리케이션 예제:

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

실행파일 `electron`의 이름을 원하는 대로 바꿀 수 있습니다.
리눅스 어플리케이션의 아이콘은 [.desktop](https://developer.gnome.org/integration-guide/stable/desktop-files.html.en) 파일을 사용하여 지정할 수 있습니다.

### 역주-자동화 

어플리케이션 배포시 Electron의 리소스를 일일이 수정하는 것은 매우 귀찮고 복잡합니다.
하지만 이 작업을 자동화 시킬 수 있는 몇가지 방법이 있습니다: 

* [electron-builder](https://github.com/loopline-systems/electron-builder)
* [electron-packager](https://github.com/maxogden/electron-packager)

## Electron 소스코드를 다시 빌드하여 리소스 수정하기

또한 Electron 소스코드를 다시 빌드할 때 어플리케이션 이름을 변경할 수 있습니다.

`GYP_DEFINES` 환경변수를 사용하여 다음과 같이 다시 빌드할 수 있습니다:

__Windows__

```bash
> set "GYP_DEFINES=project_name=myapp product_name=MyApp"
> python script\clean.py
> python script\bootstrap.py
> python script\build.py -c R -t myapp
```

__Bash__

```bash
$ export GYP_DEFINES="project_name=myapp product_name=MyApp"
$ script/clean.py
$ script/bootstrap.py
$ script/build.py -c Release -t myapp
```

### grunt-build-atom-shell

Electron의 소스코드를 수정하고 다시 빌드하는 작업은 상당히 복잡합니다.
일일이 소스코드를 수정하는 대신 [grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell)을 사용하여 빌드를 자동화 시킬 수 있습니다.

이 툴을 사용하면 자동으로 `.gyp`파일을 수정하고 다시 빌드합니다. 그리고 어플리케이션의 네이티브 Node 모듈 또한 새로운 실행파일 이름으로 매치 시킵니다.

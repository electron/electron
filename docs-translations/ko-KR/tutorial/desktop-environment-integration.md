# 데스크톱 환경 통합

어플리케이션 배포의 대상이 되는 서로 다른 운영체제 시스템의 환경에 맞춰 어플리케이션의 기능을 통합할 수 있습니다.
예를 들어 Windows에선 태스크바의 JumpList에 바로가기를 추가할 수 있고 Mac(OS X)에선 dock 메뉴에 커스텀 메뉴를 추가할 수 있습니다.

이 가이드는 Electron API를 이용하여 각 운영체제 시스템의 기능을 활용하는 방법을 설명합니다.

## 최근 사용한 문서 (Windows & OS X)

알다 싶이 Windows와 OS X는 JumpList 또는 dock 메뉴를 통해 최근 문서 리스트에 쉽게 접근할 수 있습니다.

__JumpList:__

![JumpList 최근 문서](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__어플리케이션 dock menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

파일을 최근 문서에 추가하려면 [app.addRecentDocument][addrecentdocument] API를 사용할 수 있습니다:

```javascript
var app = require('app');
app.addRecentDocument('/Users/USERNAME/Desktop/work.type');
```

그리고 [app.clearRecentDocuments][clearrecentdocuments] API로 최근 문서 리스트를 비울 수 있습니다:

```javascript
app.clearRecentDocuments();
```

### Windows에서 주의할 점

이 기능을 Windows에서 사용할 땐 운영체제 시스템에 어플리케이션에서 사용하는 파일 확장자가 등록되어 있어야 합니다.
그렇지 않은 경우 파일을 JumpList에 추가해도 추가되지 않습니다.
어플리케이션 등록에 관련된 API의 모든 내용은 [Application Registration][app-registration]에서 찾아볼 수 있습니다.

유저가 JumpList에서 파일을 클릭할 경우 클릭된 파일의 경로가 커맨드 라인 인자로 추가되어 새로운 인스턴스의 어플리케이션이 실행됩니다.

### OS X에서 주의할 점

파일이 최근 문서 메뉴에서 요청될 경우 `app` 모듈의 `open-file` 이벤트가 호출됩니다.

## 커스텀 독 메뉴 (OS X)

OS X는 개발자가 dock에 커스텀 메뉴를 만들 수 있도록 허용하고 있습니다.
보통 어플리케이션의 특정 기능 바로가기를 만들 때 사용합니다:

__Terminal.app의 dock menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

커스텀 dock menu를 설정하려면 `app.dock.setMenu` API를 사용하면 됩니다. OS X에서만 사용 가능합니다:

```javascript
var app = require('app');
var Menu = require('menu');
var dockMenu = Menu.buildFromTemplate([
  { label: 'New Window', click: function() { console.log('New Window'); } },
  { label: 'New Window with Settings', submenu: [
    { label: 'Basic' },
    { label: 'Pro'}
  ]},
  { label: 'New Command...'}
]);
app.dock.setMenu(dockMenu);
```

## 사용자 작업 (Windows)

Windows에선 JumpList의 `Tasks` 카테고리에 원하는 작업을 설정할 수 있습니다.
MSDN에선 해당 기능을 다음과 같이 설명하고 있습니다:

> 어플리케이션 작업은 프로그램의 기능 그리고 주요사양 두가지를 기반으로 유저의 행동을 예측하여 정의합니다.
> 실행할 필요가 없는 어플리케이션 작업은 작동하지 않을 때 반드시 context-free를 유지해야 합니다.
> 작업은 일반 사용자가 프로그램을 실행하거나 이메일 프로그램으로 이메일을 작성하거나 달력을 불러오고
> 워드 프로세서로 새 문서를 작성, 특정 모드, 부속 명령으로 프로그램을 실행하는 등의
> 통계적, 일반적으로 가장 많이 사용되는 작업인지를 고려해야 합니다.
> 어플리케이션 작업은 일반 유저가 필요로 하지 않는 고급 기능을 조잡하게 채우거나 등록과 같은 일회성의 작업을 포함해선 안됩니다.
> 작업에 특별 이벤트 또는 업그레이드 등의 홍보성 작업을 추가하면 안됩니다.
>
> 작업 리스트는 가능한 한 정적으로 유지되는 것을 적극 권장합니다.
> 이것은 동일한 상태 또는 응용 프로그램의 상태에 관계없이 유지되어야 합니다.
> 작업 목록은 동적으로 변경할 수 있지만 몇몇 유저는 예상하지 못한 작업 목록 변경에 혼동할 수 있다는 점을 고려해야 합니다.

__Internet Explorer의 작업:__

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

OS X의 dock menu(진짜 메뉴)와는 달리 Windows의 사용자 작업은 어플리케이션 바로 가기처럼 작동합니다.
유저가 작업을 클릭할 때 설정한 인자와 함께 새로운 어플리케이션이 실행됩니다.

사용자 작업을 설정하려면 [app.setUserTasks][setusertaskstasks] 메서드를 통해 구현할 수 있습니다:

```javascript
var app = require('app');
app.setUserTasks([
  {
    program: process.execPath,
    arguments: '--new-window',
    iconPath: process.execPath,
    iconIndex: 0,
    title: 'New Window',
    description: 'Create a new window'
  }
]);
```

작업 리스트를 비우려면 간단히 `app.setUserTasks` 메서드의 첫번째 인자에 빈 배열을 넣어 호출하면 됩니다:

```javascript
app.setUserTasks([]);
```


사용자 작업 리스트는 어플리케이션이 삭제되지 않는 한 종료되어도 태스크바에 보존됩니다. 이러한 이유로 반드시 프로그램 경로와 아이콘 경로를 지정해야 합니다.

## 섬네일 툴바

Windows에선 작업 표시줄의 어플리케이션 선택시 나오는 미리보기에 특정한 섬네일 툴바를 추가할 수 있습니다.
이 기능은 유저가 윈도우를 활성화 하지 않고 특정한 커맨드를 실행시킬 수 있도록 할 수 있습니다.

MSDN의 설명에 의하면:

> 이 툴바는 표준 툴바의 공통 컨트롤과 비슷한 역할을 합니다. 버튼은 최대 7개 까지 만들 수 있습니다.
> 각 버튼의 구조엔 ID, 이미지, 툴팁, 상태등이 정의되어있습니다. 태스크바에 구조가 전달되면 어플리케이션이
> 상태에 따라 버튼을 숨기거나, 활성화하거나, 비활성화 할 수 있습니다.
>
> 예를 들어, 윈도우 미디어 플레이어는(WMP) 기본적으로
> 미디어 플레이어가 공통적으로 사용하는 재생, 일시정지, 음소거, 정지등의 컨트롤을 포함하고 있습니다.

__Windows Media Player의 섬네일 툴바:__

![player](https://i-msdn.sec.s-msft.com/dynimg/IC420540.png)

[BrowserWindow.setThumbarButtons][setthumbarbuttons] API를 통해 어플리케이션에 섬네일 툴바를 설정할 수 있습니다:

```javascript
var BrowserWindow = require('browser-window');
var path = require('path');
var win = new BrowserWindow({
  width: 800,
  height: 600
});
win.setThumbarButtons([
  {
    tooltip: "button1",
    icon: path.join(__dirname, 'button1.png'),
    click: function() { console.log("button2 clicked"); }
  },
  {
    tooltip: "button2",
    icon: path.join(__dirname, 'button2.png'),
    flags:['enabled', 'dismissonclick'],
    click: function() { console.log("button2 clicked."); }
  }
]);
```

섬네일 툴바를 비우려면 간단히 `BrowserWindow.setThumbarButtons` API에 빈 배열을 전달하면 됩니다:

```javascript
win.setThumbarButtons([]);
```

## Unity 런처 숏컷 기능 (Linux)

Unity 환경에선 `.desktop` 파일을 수정함으로써 런처에 새로운 커스텀 엔트리를 추가할 수 있습니다. [Adding Shortcuts to a Launcher][unity-launcher] 가이드를 참고하세요.

__Audacious의 런처 숏컷:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

## Taskbar progress 기능 (Windows & Unity)

Windows에선 태스크바의 어플리케이션 버튼에 progress bar를 추가할 수 있습니다.
이 기능은 사용자가 어플리케이션의 창을 열지 않고도 어플리케이션의 작업의 상태 정보를 시각적으로 보여줄 수 있도록 해줍니다.

또한 Unity DE도 런처에 progress bar를 부착할 수 있습니다.

__태스크바 버튼의 progress bar:__

![Taskbar Progress Bar](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

__Unity 런처의 progress bar:__

![Unity Launcher](https://cloud.githubusercontent.com/assets/639601/5081747/4a0a589e-6f0f-11e4-803f-91594716a546.png)

이 기능은 [BrowserWindow.setProgressBar][setprogressbar] API를 사용하여 구현할 수 있습니다:

```javascript
var window = new BrowserWindow({...});
window.setProgressBar(0.5);
```

## 대표 파일 제시 (OS X)

OS X는 창에서 대표 파일을 설정할 수 있습니다. 타이틀바에서 파일 아이콘이 있고, 사용자가 Command-Click 또는 Control-Click 키를 누를 경우 파일 경로 팝업이 보여집니다.
또한 창의 상태도 지정할 수 있습니다. 쉽게 말해 로드된 문서의 수정여부를 타이틀바 파일 아이콘에 표시할 수 있습니다.

__대표 파일 팝업 메뉴:__

<img src="https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png" height="232" width="663" >

대표 파일 관련 API는 [BrowserWindow.setRepresentedFilename][setrepresentedfilename] 과 [BrowserWindow.setDocumentEdited][setdocumentedited]를 사용할 수 있습니다:

```javascript
var window = new BrowserWindow({...});
window.setRepresentedFilename('/etc/passwd');
window.setDocumentEdited(true);
```

[addrecentdocument]: ../api/app.md#appaddrecentdocumentpath
[clearrecentdocuments]: ../api/app.md#appclearrecentdocuments
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks
[setprogressbar]: ../api/browser-window.md#browserwindowsetprogressbarprogress
[setrepresentedfilename]: ../api/browser-window.md#browserwindowsetrepresentedfilenamefilename
[setdocumentedited]: ../api/browser-window.md#browserwindowsetdocumenteditededited
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
[setthumbarbuttons]: ../api/browser-window.md#browserwindowsetthumbarbuttonsbuttons

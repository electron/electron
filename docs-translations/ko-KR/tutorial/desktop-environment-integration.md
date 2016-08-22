# 데스크톱 환경 통합

애플리케이션 배포의 대상이 되는 서로 다른 운영체제 시스템의 환경에 맞춰 애플리케이션의
기능을 통합할 수 있습니다. 예를 들어 Windows에선 태스크바의 JumpList에 바로가기를
추가할 수 있고 Mac(macOS)에선 dock 메뉴에 커스텀 메뉴를 추가할 수 있습니다.

이 문서는 Electron API를 이용하여 각 운영체제 시스템의 기능을 활용하는 방법을
설명합니다.

## 데스크톱 알림 (Windows, Linux, macOS)

Windows, Linux, macOS 운영체제 모두 기본적으로 애플리케이션에서 유저에게 알림을 보내는
방법을 제공합니다. Electron은 [HTML5 Notification API](https://notifications.spec.whatwg.org/)를
통해 개발자가 편리하게 데스크톱 알림을 사용할 수 있는 기능을 제공합니다. 데스크톱 알림은
운영체제의 네이티브 알림 API를 사용하여 표시합니다.

**참고:** 이 API는 HTML5 API이기 때문에 렌더러 프로세스에서만 사용할 수 있습니다.

```javascript
let myNotification = new Notification('Title', {
  body: 'Lorem Ipsum Dolor Sit Amet'
});

myNotification.onclick = () => {
  console.log('Notification clicked');
};
```

위 코드를 통해 생성한 데스크톱 알림은 각 운영체제 모두 비슷한 사용자 경험을 제공하지만,
하지만 몇 가지 다른 점들이 있습니다.

### Windows

* Windows 10에선 "아무 문제 없이 잘" 작동합니다.
* Windows 8.1과 8에선 [Application User Model ID][app-user-model-id]로 바로가기를
  만들어 놔야 합니다. 이 바로가기는 반드시 시작 화면에 설치되어 있어야 합니다. 참고로
  반드시 시작 화면에 고정 할 필요는 없습니다.
* Windows 7과 그 이하 버전은 데스크톱 알림을 지원하지 않습니다.
  혹시 "풍선 팝업 알림" 기능을 찾는다면 [Tray API][tray-balloon]를 사용하세요.

또한 알림 본문의 최대 길이는 250자 입니다. Windows 개발팀에선 알림 문자열을 200자
이하로 유지하는 것을 권장합니다.

### Linux

데스크톱 알림의 구현으로 `libnotify`를 사용합니다. 따라서 [Desktop Notifications Specification][notification-spec]을
따르는 모든 데스크탑 환경에서 데스크톱 알림 기능을 사용할 수 있습니다. Cinnamon,
Enlightenment, Unity, GNOME, KDE등을 지원합니다.

### macOS

macOS에서의 데스크톱 알림은 아주 직관적입니다. 하지만 데스크톱 알림을 사용할 땐
[Apple's Human Interface guidelines regarding notifications](https://developer.apple.com/library/mac/documentation/UserExperience/Conceptual/OSXHIGuidelines/NotificationCenter.html)
가이드를 고려해야 합니다.

참고로 데스크롭 알림의 최대 길이는 256 바이트 입니다. 길이가 초과할 경우 초과한 글자가
잘립니다.

## 최근 사용한 문서 (Windows & macOS)

Windows와 macOS는 JumpList 또는 dock 메뉴를 통해 최근 문서 리스트에 쉽게 접근할 수
있습니다.

__JumpList:__

![JumpList 최근 문서](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__애플리케이션 dock menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

파일을 최근 문서에 추가하려면 [app.addRecentDocument][addrecentdocument] API를
사용할 수 있습니다:

```javascript
app.addRecentDocument('/Users/USERNAME/Desktop/work.type');
```

그리고 [app.clearRecentDocuments][clearrecentdocuments] API로 최근 문서 리스트를
비울 수 있습니다:

```javascript
app.clearRecentDocuments();
```

### Windows에서 주의할 점

이 기능을 Windows에서 사용할 땐 운영체제 시스템에 애플리케이션에서 사용하는 파일
확장자가 등록되어 있어야 합니다. 그렇지 않은 경우 파일을 JumpList에 추가해도 추가되지
않습니다. 애플리케이션 등록에 관련된 API의 모든 내용은 [Application Registration][app-registration]에서
찾아볼 수 있습니다.

유저가 JumpList에서 파일을 클릭할 경우 클릭된 파일의 경로가 커맨드 라인 인수로 추가되어
새로운 인스턴스의 애플리케이션이 실행됩니다.

### macOS에서 주의할 점

파일이 최근 문서 메뉴에서 요청될 경우 `app` 모듈의 `open-file` 이벤트가 호출됩니다.

## 커스텀 독 메뉴 (macOS)

macOS는 개발자가 dock에 커스텀 메뉴를 만들 수 있도록 허용하고 있습니다.
보통 애플리케이션의 특정 기능 바로가기를 만들 때 사용합니다:

__Terminal.app의 dock menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

커스텀 dock menu를 설정하려면 `app.dock.setMenu` API를 사용하면 됩니다.
macOS에서만 사용 가능합니다:

```javascript
const {app, Menu} = require('electron');

const dockMenu = Menu.buildFromTemplate([
  {label: 'New Window', click() { console.log('New Window'); }},
  {label: 'New Window with Settings', submenu: [
    {label: 'Basic'},
    {label: 'Pro'}
  ]},
  {label: 'New Command...'}
]);

app.dock.setMenu(dockMenu);
```

## 사용자 작업 (Windows)

Windows에선 JumpList의 `Tasks` 카테고리에 원하는 작업을 설정할 수 있습니다.
MSDN에선 해당 기능을 다음과 같이 설명하고 있습니다:

> 애플리케이션 작업은 프로그램의 기능 그리고 주요사양 두가지를 기반으로 유저의 행동을
> 예측하여 정의합니다. 실행할 필요가 없는 애플리케이션 작업은 작동하지 않을 때 반드시
> context-free를 유지해야 합니다. 작업은 일반 사용자가 프로그램을 실행하거나 이메일
> 프로그램으로 이메일을 작성하거나 달력을 불러오고, 워드 프로세서로 새 문서를 작성,
> 특정 모드, 부속 명령으로 프로그램을 실행하는 등의 통계적, 일반적으로 가장 많이
> 사용되는 작업인지를 고려해야 합니다. 애플리케이션 작업은 일반 유저가 필요로 하지
> 않는 고급 기능을 조잡하게 채우거나 등록과 같은 일회성의 작업을 포함해선 안됩니다.
> 또한 작업에 특별 이벤트 또는 업그레이드 등의 홍보성 작업을 추가하면 안됩니다.
>
> 작업 리스트는 가능한 한 정적으로 유지되는 것을 적극 권장합니다.
> 이것은 동일한 상태 또는 응용 프로그램의 상태에 관계없이 유지되어야 합니다.
> 작업 목록은 동적으로 변경할 수 있지만 몇몇 유저는 예상하지 못한 작업 목록 변경에
> 혼란을 일으킬 수 있다는 점을 고려해야 합니다.

__Internet Explorer의 작업:__

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

macOS의 dock menu(진짜 메뉴)와는 달리 Windows의 사용자 작업은 애플리케이션 바로
가기처럼 작동합니다. 유저가 작업을 클릭할 때 설정한 인수와 함께 새로운 애플리케이션이
실행됩니다.

사용자 작업을 설정하려면 [app.setUserTasks][setusertaskstasks] 메서드를 통해 구현할
수 있습니다:

```javascript
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

작업 리스트를 비우려면 간단히 `app.setUserTasks` 메서드의 첫번째 인수에 빈 배열을 넣어
호출하면 됩니다:

```javascript
app.setUserTasks([]);
```


사용자 작업 리스트는 애플리케이션이 삭제되지 않는 한 종료되어도 태스크바에 보존됩니다.
이러한 이유로 반드시 프로그램 경로와 아이콘 경로를 지정해야 합니다.

## 미리보기 툴바

Windows에선 작업 표시줄의 애플리케이션 선택시 나오는 미리보기에 특정한 미리보기 툴바를
추가할 수 있습니다. 이 기능은 유저가 윈도우를 활성화 하지 않고 특정한 커맨드를 실행시킬
수 있도록 할 수 있습니다.

MSDN의 설명에 의하면:

> 이 툴바는 표준 툴바의 공통 컨트롤과 비슷한 역할을 합니다. 버튼은 최대 7개 까지
> 만들 수 있습니다. 각 버튼의 구조엔 ID, 이미지, 툴팁, 상태 등이 정의되어있습니다.
> 작업표시줄에 구조가 전달되면 애플리케이션이 상태에 따라 버튼을 숨기거나, 활성화하거나,
> 비활성화 할 수 있습니다.
>
> 예를 들어, 윈도우 미디어 플레이어는(WMP) 미디어 플레이어가 공통적으로 사용하는
> 재생, 일시정지, 음소거, 정지등의 컨트롤을 포함하고 있습니다.

__Windows Media Player의 미리보기 툴바:__

![player](https://i-msdn.sec.s-msft.com/dynimg/IC420540.png)

[BrowserWindow.setThumbarButtons][setthumbarbuttons] API를 통해 애플리케이션에
미리보기 툴바를 설정할 수 있습니다:

```javascript
const {BrowserWindow} = require('electron');
const path = require('path');

let win = new BrowserWindow({
  width: 800,
  height: 600
});

win.setThumbarButtons([
  {
    tooltip: 'button1',
    icon: path.join(__dirname, 'button1.png'),
    click() { console.log('button1 clicked'); }
  },
  {
    tooltip: 'button2',
    icon: path.join(__dirname, 'button2.png'),
    flags:['enabled', 'dismissonclick'],
    click() { console.log('button2 clicked.'); }
  }
]);
```

미리보기 툴바를 비우려면 간단히 `BrowserWindow.setThumbarButtons` API에 빈 배열을
전달하면 됩니다:

```javascript
win.setThumbarButtons([]);
```

## Unity 런처 숏컷 기능 (Linux)

Unity 환경에선 `.desktop` 파일을 수정함으로써 런처에 새로운 커스텀 엔트리를 추가할 수
있습니다. [Adding Shortcuts to a Launcher][unity-launcher] 가이드를 참고하세요.

__Audacious의 런처 숏컷:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

## 작업 표시줄 안의 프로그래스 바 (Windows, macOS, Unity)

Windows에선 작업 표시줄의 애플리케이션 버튼에 프로그래스 바를 추가할 수 있습니다.
이 기능은 사용자가 애플리케이션의 창을 열지 않고도 애플리케이션의 작업의 상태 정보를
시각적으로 보여줄 수 있도록 해줍니다.

macOS에선 프로그래스바가 dock 아이콘의 일부에 표시됩니다.

또한 Unity DE도 런처에 프로그래스 바를 부착할 수 있습니다.

__작업 표시줄 버튼의 프로그래스 바:__

![Taskbar Progress Bar](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

이 기능은 [BrowserWindow.setProgressBar][setprogressbar] API를 사용하여 구현할 수
있습니다:

```javascript
let win = new BrowserWindow({...});
win.setProgressBar(0.5);
```

## 작업 표시줄의 아이콘 오버레이 (Windows)

Windows에선 작업 표시줄 버튼에 애플리케이션의 상태를 표시하는 작은 오버레이를 사용할
수 있습니다. MSDN에서 인용하자면 (영문):

> Icon overlays serve as a contextual notification of status, and are intended
> to negate the need for a separate notification area status icon to communicate
> that information to the user. For instance, the new mail status in Microsoft
> Outlook, currently shown in the notification area, can now be indicated
> through an overlay on the taskbar button. Again, you must decide during your
> development cycle which method is best for your application. Overlay icons are
> intended to supply important, long-standing status or notifications such as
> network status, messenger status, or new mail. The user should not be
> presented with constantly changing overlays or animations.

__작업 표시줄 버튼 위의 오버레이:__

![작업 표시줄 버튼 위의 오버레이](https://i-msdn.sec.s-msft.com/dynimg/IC420441.png)

윈도우에 오버레이 아이콘을 설정하려면 [BrowserWindow.setOverlayIcon][setoverlayicon]
API를 사용할 수 있습니다:

```javascript
let win = new BrowserWindow({...});
win.setOverlayIcon('path/to/overlay.png', 'Description for overlay');
```

## 대표 파일 제시 (macOS)

macOS는 창에서 대표 파일을 설정할 수 있습니다. 타이틀바에서 파일 아이콘이 있고, 사용자가
Command-Click 또는 Control-Click 키를 누를 경우 파일 경로 팝업이 보여집니다. 또한
창의 상태도 지정할 수 있습니다. 다시 말해 로드된 문서의 수정 여부를 제목의 파일
아이콘에 표시할 수 있습니다.

__대표 파일 팝업 메뉴:__

<img src="https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png" height="232" width="663" >

대표 파일 관련 API는 [BrowserWindow.setRepresentedFilename][setrepresentedfilename] 과
[BrowserWindow.setDocumentEdited][setdocumentedited]를 사용할 수 있습니다:

```javascript
let win = new BrowserWindow({...});
win.setRepresentedFilename('/etc/passwd');
win.setDocumentEdited(true);
```

## 파일을 윈도우 밖으로 드래그할 수 있도록 만들기

파일을 조작하는 특정 종류의 애플리케이션들에서 파일을 Electron에서 다른 애플리케이션으로
드래그할 수 있는 기능은 중요합니다. 이 기능을 구현하려면 애플리케이션에서
`ondragstart` 이벤트가 발생했을 때 `webContents.startDrag(item)` API를 호출해야
합니다:

웹 페이지에서:

```html
<a href="#" id="drag">item</a>
<script type="text/javascript" charset="utf-8">
  document.getElementById('drag').ondragstart = (event) => {
    event.preventDefault()
    ipcRenderer.send('ondragstart', '/path/to/item')
  }
</script>
```

메인 프로세스에서:

```javascript
ipcMain.on('ondragstart', (event, filePath) => {
  event.sender.startDrag({
    file: filePath,
    icon: '/path/to/icon.png'
  })
})
```

[addrecentdocument]: ../api/app.md#appaddrecentdocumentpath-os-x-windows
[clearrecentdocuments]: ../api/app.md#appclearrecentdocuments-os-x-windows
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks-windows
[setprogressbar]: ../api/browser-window.md#winsetprogressbarprogress
[setoverlayicon]: ../api/browser-window.md#winsetoverlayiconoverlay-description-windows-7
[setrepresentedfilename]: ../api/browser-window.md#winsetrepresentedfilenamefilename-os-x
[setdocumentedited]: ../api/browser-window.md#winsetdocumenteditededited-os-x
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
[setthumbarbuttons]: ../api/browser-window.md#winsetthumbarbuttonsbuttons-windows-7
[tray-balloon]: ../api/tray.md#traydisplayballoonoptions-windows
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[notification-spec]: https://developer.gnome.org/notification-spec/

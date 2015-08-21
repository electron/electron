# menu

`Menu` 클래스는 어플리케이션 메뉴와 컨텍스트 메뉴를 만들 때 사용할 수 있습니다.
각 메뉴는 여러 개의 메뉴 아이템으로 구성되어 있으며 서브 메뉴를 가질 수도 있습니다.

다음 예제는 웹 페이지 내에서 [remote](remote-ko.md) 모듈을 활용하여 동적으로 메뉴를 생성하는 예제입니다.
그리고 이 예제에서 만들어진 메뉴는 유저가 페이지에서 오른쪽 클릭을 할 때 마우스 위치에 팝업으로 표시됩니다:

```html
<!-- index.html -->
<script>
var remote = require('remote');
var Menu = remote.require('menu');
var MenuItem = remote.require('menu-item');

var menu = new Menu();
menu.append(new MenuItem({ label: 'MenuItem1', click: function() { console.log('item 1 clicked'); } }));
menu.append(new MenuItem({ type: 'separator' }));
menu.append(new MenuItem({ label: 'MenuItem2', type: 'checkbox', checked: true }));

window.addEventListener('contextmenu', function (e) {
  e.preventDefault();
  menu.popup(remote.getCurrentWindow());
}, false);
</script>
```

다음 예제는 template API를 활용하여 어플리케이션 메뉴를 만드는 간단한 예제입니다:

**Windows 와 Linux 주의:** 각 메뉴 아이템의 `selector` 멤버는 Mac 운영체제 전용입니다. [Accelerator 옵션](https://github.com/atom/electron/blob/master/docs/api/accelerator-ko.md)

```html
<!-- index.html -->
<script>
var remote = require('remote');
var Menu = remote.require('menu');
var template = [
  {
    label: 'Electron',
    submenu: [
      {
        label: 'About Electron',
        selector: 'orderFrontStandardAboutPanel:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Services',
        submenu: []
      },
      {
        type: 'separator'
      },
      {
        label: 'Hide Electron',
        accelerator: 'CmdOrCtrl+H',
        selector: 'hide:'
      },
      {
        label: 'Hide Others',
        accelerator: 'CmdOrCtrl+Shift+H',
        selector: 'hideOtherApplications:'
      },
      {
        label: 'Show All',
        selector: 'unhideAllApplications:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Quit',
        accelerator: 'CmdOrCtrl+Q',
        selector: 'terminate:'
      },
    ]
  },
  {
    label: 'Edit',
    submenu: [
      {
        label: 'Undo',
        accelerator: 'CmdOrCtrl+Z',
        selector: 'undo:'
      },
      {
        label: 'Redo',
        accelerator: 'Shift+CmdOrCtrl+Z',
        selector: 'redo:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Cut',
        accelerator: 'CmdOrCtrl+X',
        selector: 'cut:'
      },
      {
        label: 'Copy',
        accelerator: 'CmdOrCtrl+C',
        selector: 'copy:'
      },
      {
        label: 'Paste',
        accelerator: 'CmdOrCtrl+V',
        selector: 'paste:'
      },
      {
        label: 'Select All',
        accelerator: 'CmdOrCtrl+A',
        selector: 'selectAll:'
      }
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'CmdOrCtrl+R',
        click: function() { remote.getCurrentWindow().reload(); }
      },
      {
        label: 'Toggle DevTools',
        accelerator: 'Alt+CmdOrCtrl+I',
        click: function() { remote.getCurrentWindow().toggleDevTools(); }
      },
    ]
  },
  {
    label: 'Window',
    submenu: [
      {
        label: 'Minimize',
        accelerator: 'CmdOrCtrl+M',
        selector: 'performMiniaturize:'
      },
      {
        label: 'Close',
        accelerator: 'CmdOrCtrl+W',
        selector: 'performClose:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Bring All to Front',
        selector: 'arrangeInFront:'
      }
    ]
  },
  {
    label: 'Help',
    submenu: []
  }
];

menu = Menu.buildFromTemplate(template);

Menu.setApplicationMenu(menu);
</script>
```

## Class: Menu

### new Menu()

새로운 메뉴를 생성합니다.

### Class Method: Menu.setApplicationMenu(menu)

* `menu` Menu

지정한 `menu`를 이용하여 어플리케이션 메뉴를 만듭니다. OS X에선 상단바에 표시되며 Windows와 Linux에선 각 창의 상단에 표시됩니다.

### Class Method: Menu.sendActionToFirstResponder(action)

* `action` String

`action`을 어플리케이션의 first responder에 전달합니다.
이 함수는 Cocoa 메뉴 동작을 에뮬레이트 하는데 사용되며 보통 `MenuItem`의 `selector` 속성에 사용됩니다.

**알림:** 이 함수는 OS X에서만 사용할 수 있습니다.

### Class Method: Menu.buildFromTemplate(template)

* `template` Array

기본적으로 `template`는 [MenuItem](menu-item-ko.md)을 생성할 때 사용하는 `options`의 배열입니다. 사용법은 위에서 설명한 것과 같습니다.

또한 `template`에는 다른 속성도 추가할 수 있으며 메뉴가 만들어질 때 해당 메뉴 아이템의 프로퍼티로 변환됩니다.

### Menu.popup(browserWindow, [x, y])

* `browserWindow` BrowserWindow
* `x` Number
* `y` Number

메뉴를 `browserWindow` 안에서 팝업으로 표시합니다.
옵션으로 메뉴를 표시할 `(x,y)` 좌표를 임의로 지정할 수 있습니다. 따로 지정하지 않은 경우 마우스 커서 위치에 표시됩니다.

### Menu.append(menuItem)

* `menuItem` MenuItem

메뉴의 리스트 끝에 `menuItem`을 삽입합니다.

### Menu.insert(pos, menuItem)

* `pos` Integer
* `menuItem` MenuItem

`pos` 위치에 `menuItem`을 삽입합니다.

### Menu.items

메뉴가 가지고 있는 메뉴 아이템들의 배열입니다.

## OS X 어플리케이션 메뉴에 대해 알아 둬야 할 것들

OS X에선 Windows, Linux와 달리 완전히 다른 어플리케이션 메뉴 스타일을 가지고 있습니다.
어플리케이션을 네이티브처럼 작동할 수 있도록 하기 위해선 다음의 몇 가지 유의 사항을 숙지해야 합니다.

### 기본 메뉴

OS X엔 `Services`나 `Windows`와 같은 많은 시스템 지정 기본 메뉴가 있습니다.
기본 메뉴를 만들려면 다음 중 하나를 메뉴의 라벨로 지정하기만 하면 됩니다.
그러면 Electron이 자동으로 인식하여 해당 메뉴를 기본 메뉴로 만듭니다:

* `Window`
* `Help`
* `Services`

### 기본 메뉴 아이템 동작

OS X는 몇몇의 메뉴 아이템에 대해 `About xxx`, `Hide xxx`, `Hide Others`와 같은 기본 동작을 제공하고 있습니다. (`selector`라고 불립니다)
메뉴 아이템의 기본 동작을 지정하려면 메뉴 아이템의 `selector` 속성을 사용하면 됩니다.

### 메인 메뉴의 이름

OS X에선 지정한 어플리케이션 메뉴에 상관없이 메뉴의 첫번째 라벨은 언제나 어플리케이션의 이름이 됩니다.
어플리케이션 이름을 변경하려면 앱 번들내의 `Info.plist` 파일을 수정해야합니다.
자세한 내용은 [About Information Property List Files](https://developer.apple.com/library/ios/documentation/general/Reference/InfoPlistKeyReference/Articles/AboutInformationPropertyListFiles.html)을 참고하세요.

## 메뉴 아이템 위치

`Menu.buildFromTemplate`로 메뉴를 만들 때 `position`과 `id`를 사용해서 아이템의 위치를 지정할 수 있습니다.

`MenuItem`의 `position` 속성은 `[placement]=[id]`와 같은 형식을 가지며 `placement`는
`before`, `after`, `endof` 속성 중 한가지를 사용할 수 있고 `id`는 메뉴 아이템이 가지는 유일 ID 입니다:

* `before` - 이 아이템을 지정한 id 이전의 위치에 삽입합니다. 만약 참조된 아이템이 없을 경우 메뉴의 맨 뒤에 삽입됩니다.
* `after` - 이 아이템을 지정한 id 다음의 위치에 삽입합니다. 만약 참조된 아이템이 없을 경우 메뉴의 맨 뒤에 삽입됩니다.
* `endof` - 이 아이템을 id의 논리 그룹에 맞춰서 각 그룹의 항목 뒤에 삽입합니다. (그룹은 분리자 아이템에 의해 만들어집니다)
  만약 참조된 아이템의 분리자 그룹이 존재하지 않을 경우 지정된 id로 새로운 분리자 그룹을 만든 후 해당 그룹의 뒤에 삽입됩니다.

위치를 지정한 아이템의 뒤에 위치가 지정되지 않은 아이템이 있을 경우 해당 아이템의 위치가 지정되기 전까지 이전에 위치가 지정된 아이템의 위치 지정을 따릅니다.
이에 따라 위치를 이동하고 싶은 특정 그룹의 아이템들이 있을 경우 해당 그룹의 맨 첫번째 메뉴 아이템의 위치만을 지정하면 됩니다.

### 예제

메뉴 템플릿:

```javascript
[
  {label: '4', id: '4'},
  {label: '5', id: '5'},
  {label: '1', id: '1', position: 'before=4'},
  {label: '2', id: '2'},
  {label: '3', id: '3'}
]
```

메뉴:

```
- 1
- 2
- 3
- 4
- 5
```

메뉴 템플릿:

```javascript
[
  {label: 'a', position: 'endof=letters'},
  {label: '1', position: 'endof=numbers'},
  {label: 'b', position: 'endof=letters'},
  {label: '2', position: 'endof=numbers'},
  {label: 'c', position: 'endof=letters'},
  {label: '3', position: 'endof=numbers'}
]
```

메뉴:

```
- ---
- a
- b
- c
- ---
- 1
- 2
- 3
```

# MenuItem

`menu-item` 모듈은 어플리케이션 또는 컨텍스트 [`menu`](menu.md)에 항목 아이템을
추가할 수 있도록 관련 클래스를 제공합니다.

[`menu`](menu.md)에서 예제를 확인할 수 있습니다.

## Class: MenuItem

`MenuItem` 인스턴스 객체에서 사용할 수 있는 메서드입니다:

### new MenuItem(options)

* `options` Object
  * `click` Function - 메뉴 아이템이 클릭될 때 `click(menuItem, browserWindow)`
    형태로 호출 되는 콜백 함수
  * `role` String - 메뉴 아이템의 액션을 정의합니다. 이 속성을 지정하면 `click`
    속성이 무시됩니다.
  * `type` String - `MenuItem`의 타입 `normal`, `separator`, `submenu`,
    `checkbox` 또는 `radio`를 사용할 수 있습니다. 만약 값이 `Menu`가 아니면
    `Menu.buildFromTemplate`를 통해 자동으로 변환됩니다.
  * `label` String
  * `sublabel` String
  * `accelerator` [Accelerator](accelerator.md)
  * `icon` [NativeImage](native-image.md)
  * `enabled` Boolean
  * `visible` Boolean
  * `checked` Boolean
  * `submenu` Menu - 보조메뉴를 설정합니다. `type`이 `submenu`일 경우 반드시
    설정해야 합니다. 일반 메뉴 아이템일 경우 생략할 수 있습니다.     
  * `id` String - 현재 메뉴 아이템에 대해 유일키를 지정합니다. 이 키는 이후
    `position` 옵션에서 사용할 수 있습니다.
  * `position` String - 미리 지정한 `id`를 이용하여 메뉴 아이템의 위치를 세밀하게
    조정합니다.

메뉴 아이템을 생성할 때, 다음 목록과 일치하는 표준 동작은 수동으로 직접 구현하는 대신
`role` 속성을 지정하여 고유 OS 경험을 최대한 살릴 수 있습니다.

`role` 속성은 다음 값을 가질 수 있습니다:

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `selectall`
* `minimize` - 현재 윈도우를 최소화합니다
* `close` - 현재 윈도우를 닫습니다

OS X에서의 `role`은 다음 값을 추가로 가질 수 있습니다:

* `about` - `orderFrontStandardAboutPanel` 액션에 대응
* `hide` - `hide` 액션에 대응
* `hideothers` - `hideOtherApplications` 액션에 대응
* `unhide` - `unhideAllApplications` 액션에 대응
* `front` - `arrangeInFront` 액션에 대응
* `window` - 부 메뉴를 가지는 "Window" 메뉴
* `help` - 부 메뉴를 가지는 "Help" 메뉴
* `services` - 부 메뉴를 가지는 "Services" 메뉴

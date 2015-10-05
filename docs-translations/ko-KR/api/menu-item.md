# MenuItem

`menu-item` 모듈은 어플리케이션 또는 컨텍스트 [`menu`](menu.md)에 아이템을 추가할 수 있도록 관련 클래스를 제공합니다.

[`menu`](menu.md)에서 예제를 확인할 수 있습니다.

## Class: MenuItem

`MenuItem` 인스턴스 객체에서 사용할 수 있는 메서드입니다:

### new MenuItem(options)

* `options` Object
  * `click` Function - 메뉴 아이템이 클릭될 때 `click(menuItem, browserWindow)` 형태로 호출 되는 콜백 함수
  * `role` String - 메뉴 아이템의 액션을 정의합니다. 이 속성을 지정하면 `click` 속성이 무시됩니다.
  * `type` String - `MenuItem`의 타입 `normal`, `separator`, `submenu`, `checkbox` 또는 `radio` 사용가능
  * `label` String
  * `sublabel` String
  * `accelerator` [Accelerator](accelerator.md)
  * `icon` [NativeImage](native-image.md)
  * `enabled` Boolean
  * `visible` Boolean
  * `checked` Boolean
  * `submenu` Menu - 보조메뉴를 설정합니다. `type`이 `submenu`일 경우 반드시 설정해야합니다. 일반 메뉴 아이템일 경우 생략할 수 있습니다.     
  * `id` String - 현재 메뉴 아이템에 대해 유일키를 지정합니다. 이 키는 이후 `position` 옵션에서 사용할 수 있습니다.
  * `position` String - 미리 지정한 `id`를 이용하여 메뉴 아이템의 위치를 세밀하게 조정합니다.

When creating menu items, it is recommended to specify `role` instead of
manually implementing the behavior if there is matching action, so menu can have
best native experience.

The `role` property can have following values:

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `selectall`
* `minimize` - Minimize current window
* `close` - Close current window

On OS X `role` can also have following additional values:

* `about` - Map to the `orderFrontStandardAboutPanel` action
* `hide` - Map to the `hide` action
* `hideothers` - Map to the `hideOtherApplications` action
* `unhide` - Map to the `unhideAllApplications` action
* `front` - Map to the `arrangeInFront` action
* `window` - The submenu is a "Window" menu
* `help` - The submenu is a "Help" menu
* `services` - The submenu is a "Services" menu

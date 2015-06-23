# menu-item

## Class: MenuItem

### new MenuItem(options)

* `options` Object
  * `click` Function - 메뉴 아이템이 클릭될 때 호출되는 콜백함수
  * `selector` String - First Responder가 클릭될 때 호출 되는 선택자 (OS X 전용)
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

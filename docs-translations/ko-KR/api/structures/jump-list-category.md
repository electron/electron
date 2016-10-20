# JumpListCategory Object

* `type` String - 다음 중 하나:
  * `tasks` - 이 카테고리의 항목은 표준 `Tasks` 카테고리에 위치할 것 입니다.
    이 카테고리는 하나만 존재하며, 항상 점프 목록의 하단에 보여집니다.
  * `frequent` - 앱에 의해 자주 열린 파일의 목록을 보여줍니다. 카테고리의
    이름과 항목들은 윈도우에 의해 설정 됩니다.
  * `recent` - 앱에 의해 최근에 열린 파일의 목록을 보여줍니다. 카테고리의
    이름과 항목들은 윈도우에 의해 설정 됩니다. `app.addRecentDocument(path)` 을
    사용하면 간접적으로 이 카테고리에 항목이 추가될 것 입니다.
  * `custom` - 작업 또는 파일 링크를 보여주며, 앱에 의해 `name` 설정되어야 합니다.
* `name` String - `type` 이 `custom` 이면 꼭 설정되어야 하고, 그 외는 생략합니다.
* `items` JumpListItem[] - `type` 이 `tasks` 또는 `custom` 이면 [`JumpListItem`]
  (jump-list-item.md) 객체의 배열, 그 외는 생략합니다.

**참고:** `JumpListCategory` 객체가 `type`, `name` 속성 둘 다 없다면 `type` 은
`tasks` 로 가정합니다. `name` 속성이 설정되었지만 `type` 속성이 생략된 경우
`type` 은 `custom` 으로 가정합니다.

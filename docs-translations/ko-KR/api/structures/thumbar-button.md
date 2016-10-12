# ThumbarButton Object

* `icon` [NativeImage](../native-image.md) - The icon showing in thumbnail
  toolbar.
* `click` Function
* `tooltip` String (optional) - 버튼 툴팁 글자.
* `flags` String[] (optional) - 버튼의 컨트롤 특유의 상태와 행동. 기본값은
  `['enabled']`.

`flags` 는 다음 `String` 들을 포함할 수 있는 배열입니다:

* `enabled` - 버튼이 활성화되어 사용자가 이용할 수 있다.
* `disabled` - 버튼이 비활성화 되어있습니다. 존재하지만 사용자 동작에 반응할 수
  없는 시각 상태를 표시합니다.
* `dismissonclick` - 버튼이 눌렸을 때 미리보기 창을 즉시 닫습니다.
* `nobackground` - 버튼 테두리를 그리지 않고 이미지만 사용합니다.
* `hidden` - 버튼이 사용자에게 보이지 않습니다.
* `noninteractive` - 버튼이 활성화되어있지만 상호작용하지 않습니다; 눌려지지않은
  버튼 상태로 그려집니다. 이 값은 버튼이 알림에 사용되는 경우를 위한 것입니다.

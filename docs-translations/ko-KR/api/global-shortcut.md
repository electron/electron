# global-shortcut

`global-shortcut` 모듈은 운영체제의 전역 키보드 단축키를 등록/해제 하는 방법을 제공합니다.
이 모듈을 사용하여 사용자가 다양한 작업을 편하게 할 수 있도록 단축키를 정의 할 수 있습니다.

**참고:** 등록된 단축키는 어플리케이션이 백그라운드로 작동(창이 포커스 되지 않음) 할 때도 계속해서 작동합니다.
이 모듈은 `app` 모듈의 `ready` 이벤트 이전에 사용할 수 없습니다.

```javascript
var app = require('app');
var globalShortcut = require('global-shortcut');

app.on('ready', function() {
  // 'ctrl+x' 단축키를 리스너에 등록합니다.
  var ret = globalShortcut.register('ctrl+x', function() {
    console.log('ctrl+x is pressed');
  });

  if (!ret) {
    console.log('registration failed');
  }

  // 단축키가 등록되었는지 확인합니다.
  console.log(globalShortcut.isRegistered('ctrl+x'));
});

app.on('will-quit', function() {
  // 단축키의 등록을 해제합니다.
  globalShortcut.unregister('ctrl+x');

  // 모든 단축키의 등록을 해제합니다.
  globalShortcut.unregisterAll();
});
```

## Methods

`global-shortcut` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `globalShortcut.register(accelerator, callback)`

* `accelerator` [Accelerator](accelerator.md)
* `callback` Function

`accelerator`로 표현된 전역 단축키를 등록합니다. 유저로부터 등록된 단축키가 눌렸을 경우 `callback` 함수가 호출됩니다.

### `globalShortcut.isRegistered(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

지정된 `accelerator` 단축키가 등록되었는지 여부를 확인합니다. 반환값은 boolean(true, false) 입니다.

### `globalShortcut.unregister(accelerator)`

* `accelerator` [Accelerator](accelerator.md)

`accelerator`에 해당하는 전역 단축키를 등록 해제합니다.

### `globalShortcut.unregisterAll()`

모든 전역 단축키의 등록을 해제합니다.

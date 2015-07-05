# `window.open` 함수

`window.open` 함수가 호출되면 새 창에서 새로운 페이지를 불러옵니다.
이 창은 `url`로 부터 만들어진 `BrowserWindow`의 새 인스턴스이며 본 객체 대신 페이지의 컨트롤이 제한된 프록시 객체를 반환합니다.

프록시 객체는 기존의 웹 페이지와 호환될 수 있도록 일부 제한된 표준 기능만 가지고 있습니다.
창의 모든 컨트롤을 가지려면 `BrowserWindow`를 직접 생성하여 작업해야 합니다.

## window.open(url, [frameName[, features]])

* `url` String
* `frameName` String
* `features` String

`BrowserWindowProxy` 클래스의 객체를 반환하는 새로운 윈도우를 생성합니다.

## window.opener.postMessage(message, targetOrigin)

* `message` String
* `targetOrigin` String

부모 윈도우에 메시지를 보냅니다. 특정한 origin을 지정할 수도 있으며 `*`를 지정하면 따로 origin 설정을 사용하지 않습니다.

## Class: BrowserWindowProxy

### BrowserWindowProxy.blur()

자식 윈도우의 포커스를 해제합니다.

### BrowserWindowProxy.close()

자식 윈도우를 강제로 닫습니다. unload 이벤트가 발생하지 않습니다.

Forcefully closes the child window without calling its unload event.

### BrowserWindowProxy.closed

자식 윈도우가 닫히면 true로 설정됩니다.

### BrowserWindowProxy.eval(code)

* `code` String

자식 윈도우에서 특정 스크립트를 실행합니다.

### BrowserWindowProxy.focus()

자식 윈도우에 포커스를 맞춥니다. (창을 맨 앞으로 가져옵니다)

### BrowserWindowProxy.postMessage(message, targetOrigin)

* `message` String
* `targetOrigin` String

자식 윈도우에 메시지를 보냅니다. 특정한 origin을 지정할 수도 있으며 `*`를 지정하면 따로 origin 설정을 사용하지 않습니다.

참고로 자식 윈도우의 `window.opener` 객체에는 다른 속성 없이 이 함수 하나만 구현되어 있습니다.

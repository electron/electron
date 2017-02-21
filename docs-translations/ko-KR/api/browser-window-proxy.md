## Class: BrowserWindowProxy

> 자식 브라우저 윈도우를 제어합니다.

프로세스: [렌더러 프로세스](../tutorial/quick-start.md#renderer-process)

`BrowserWindowProxy` 객체는 `window.open`에서 반환되며, 자식 윈도우와 함께 제한된 기능을 제공합니다.

### Instance Methods (인스턴스 메소드)

`BrowserWindowProxy` 객체는 다음과 같은 Instance Methods(인스턴스 메소드)가 있습니다.

#### `win.blur()`

자식 윈도우에서 포커스를 제거합니다.

#### `win.close()`

unload 이벤트를 호출하지 않고, 자식 윈도우를 강제로 종료합니다.

#### `win.eval(code)`

* `code` String

자식 윈도우에서 코드를 실행합니다.

#### `win.focus()`

자식 윈도우을 강조합니다 (윈도우를 앞으로 가져옵니다).
Focuses the child window (brings the window to front).

#### `win.print()`

자식 윈도우에서 print dialog를 호출합니다.

#### `win.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

지정된 origin과 함께 메시지를 보냅니다. 별도의 origin이 지정되어 있지 않다면 `*`을 작성합니다.

이 메소드들 외에도 자식 윈도우는 속성이 없는 단일 메소드가 있는 `window.opener` 객체를 구현합니다.

### Instance Properties (인스턴스 속성)

`BrowserWindowProxy` 객체는 다음과 같은 instance properties(인스턴스 속성)를 가집니다.

#### `win.closed`

자식 윈도우가 닫힌 후에 true로 설정된 Boolean값 입니다.


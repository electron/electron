# Electron 문서 스타일 가이드

[Electron 문서 읽기](#electron-문서-읽기) 와 [Electron 문서 작성하기](#electron-문서-작성하기) 중 이해가 필요한 부분을 찾아 참고하세요:

## Electron 문서 작성하기

Electron 문서를 작성하는 규칙은 다음과 같습니다.

- `h1` 제목은 페이지당 한 개만 사용할 수 있습니다.
- 코드 블럭에서 터미널 언어 선택시 `cmd` 대신 `bash`를 사용합니다. (syntax highlighter를 사용하기 위해서)
- 문서의 `h1` 제목은 반드시 현재 객체 이름과 같게 해야 합니다. (예시: `browser-window` → `BrowserWindow`)
 - 하이픈(-)으로 구분되었던 어떻게 되었던 간에 예시와 같이 작성합니다.
- 헤더 밑에 헤더를 바로 사용하지 않습니다. 한 줄이라도 좋으니 헤더와 헤더 사이에 설명 추가합니다.
- 메서드 헤더는 `code` 틱으로 표시합니다.
- 이벤트 헤더는 한 '따옴표'로 표시합니다.
- 리스트를 2 단계 이상 중첩하지 않습니다. (안타깝게도 markdown 랜더러가 지원하지 않습니다)
- 섹션에 대한 제목을 추가합니다: Events, Class 메서드 그리고 인스턴스 메서드등.
- 어떤 '것'의 사용 결과를 설명할 때 '될 것입니다' 형식을 사용하여 설명합니다.
- 이벤트와 메서드에는 `h3` 헤더를 사용합니다.
- 선택적 인수는 `function (required[, optional])` 형식으로 작성합니다.
- 선택적 인수는 목록에서 호출되면 표시합니다.
- 문장의 길이는 한 줄당 80 칸을 유지합니다.
- 플랫폼 특정 메서드 헤더는 이탈릭체로 표시합니다.
 - ```### `method(foo, bar)` _OS X_```
- 'on' 표현 대신 'in the ___ process' 형식의 표현을 지향합니다.

### 번역된 참조 문서

번역된 Electron의 참조 문서는 `docs-translations` 디렉터리에 있습니다.

아직 번역되지 않은 언어를 추가하려면 (일부분 포함):

- 언어의 약어(예: en, ja, ko등)로 서브 디렉터리를 만듭니다.
- 서브 디렉터리에 `docs` 디렉터리를 복사합니다. 파일 이름과 디렉터리 구조는 모두 유지합니다.
- 문서를 번역합니다.
- `README.md`에 번역한 문서의 링크를 추가하고 업데이트 합니다.
- 메인 Electron의 [README](https://github.com/atom/electron#documentation-translations)에 번역된 디렉터리의 링크를 추가합니다.

## Electron 문서 읽기

Electron 문서 구조를 이해하는 데 참고할 수 있는 유용한 도움말입니다.

### Methods

[Method](https://developer.mozilla.org/en-US/docs/Glossary/Method) 문서의 예제입니다:

---

`methodName(required[, optional]))`

* `require` String, **필수**
* `optional` Integer

---

메서드 이름은 인수가 무엇을 받는지에 따라 결정됩니다. 선택적 인수는 브라켓([, ])으로 묶어
이 인수가 다른 인수뒤에서 선택적으로 사용될 수 있다는 것을 표시합니다.

메서드 이름 하단에선 각 인수에 대해 자세한 설명을 합니다.
인수의 타입은 일반적인 타입 중 하나를 받거나:
[`String`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String),
[`Number`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number),
[`Object`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object),
[`Array`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array)
와 같은 일반적으로 쓰이는 타입 중 하나를 받거나 Electron의 [`webContent`](api/web-content.md)같은 커스텀 타입을 받습니다.

### Events

[Event](https://developer.mozilla.org/en-US/docs/Web/API/Event) 문서의 예제입니다:

---

Event: 'wake-up'

Returns:

* `time` String

---

이벤트는 `.on` 리스너 메서드로 사용할 수 있습니다. 만약 이벤트에서 값을 반환한다면 문서에서 표시된 대로
일정한 타입의 값을 반환합니다. 이벤트를 처리하거나 응답하려 한다면 다음과 같이 사용할 수 있습니다:

```javascript
Alarm.on('wake-up', function(time) {
  console.log(time)
})
```

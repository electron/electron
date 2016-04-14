# 코딩 스타일

이 가이드는 Electron의 코딩 스타일에 관해 설명합니다.

`npm run lint`를 실행하여 `cpplint`와 `eslint`를 통해 어떤 코딩 스타일 이슈를 확인할
수 있습니다.

## C++과 Python

C++과 Python 스크립트는 Chromium의
[코딩 스타일](http://www.chromium.org/developers/coding-style)을 따릅니다. 파이선
스크립트 `script/cpplint.py`를 사용하여 모든 파일이 해당 코딩스타일에 맞게 코딩 했는지
확인할 수 있습니다.

Python 버전은 2.7을 사용합니다.

C++ 코드는 많은 Chromium의 추상화와 타입을 사용합니다. 따라서 Chromium 코드에 대해 잘
알고 있어야 합니다. 이와 관련하여 시작하기 좋은 장소로 Chromium의
[Important Abstractions and Data Structures](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures)
문서가 있습니다. 이 문서에선 몇가지 특별한 타입과 스코프 타입(스코프 밖으로 나가면
자동으로 메모리에서 할당을 해제합니다. 스마트 포인터와 같습니다) 그리고 로깅 메커니즘
등을 언급하고 있습니다.

## JavaScript

* [표준](http://npm.im/standard) JavaScript 코딩 스타일을 사용합니다.
* Google의 코딩 스타일에도 맞추기 위해 파일의 끝에는 **절대** 개행을 삽입해선 안됩니다.
* 파일 이름의 공백은 `_`대신에 `-`을 사용하여야 합니다. 예를 들어
`file_name.js`를 `file-name.js`로 고쳐야 합니다. 왜냐하면
[github/atom](https://github.com/github/atom)에서 사용되는 모듈의 이름은 보통
`module-name` 형식이기 때문입니다. 이 규칙은 '.js' 파일에만 적용됩니다.
* 적절한 곳에 새로운 ES6/ES2015 문법을 사용해도 됩니다.
  * [`const`](https://developer.mozilla.org/ko/docs/Web/JavaScript/Reference/Statements/const)
    는 requires와 다른 상수에 사용합니다
  * [`let`](https://developer.mozilla.org/ko/docs/Web/JavaScript/Reference/Statements/let)
    은 변수를 정의할 때 사용합니다
  * [Arrow functions](https://developer.mozilla.org/ko/docs/Web/JavaScript/Reference/Functions/Arrow_functions)
    는 `function () { }` 표현 대신에 사용합니다
  * [Template literals](https://developer.mozilla.org/ko/docs/Web/JavaScript/Reference/Template_literals)
    는 `+`로 문자열을 합치는 대신 사용합니다.

## 이름 짓기

Electron API는 Node.js와 비슷한 명명법을 사용합니다:

- `BrowserWindow`와 같은 모듈 자체를 뜻하는 이름은, `CamelCase`를 사용합니다.
- `globalShortcut`과 같은 API의 세트일 땐, `mixedCase`를 사용합니다.
- API가 객체의 속성일 경우, 그리고 `win.webContents`와 같이 충분히 복잡하고 분리된
  부분일 경우, `mixedCase`를 사용합니다.
- 다른 모듈이 아닌 API를 구현할 땐, `<webview> Tag` 또는 `Process Object`와 같이
  단순하고 자연스러운 제목을 사용합니다.

새로운 API를 만들 땐 jQuery의 one-function 스타일 대신 getter, setter스타일을
사용해야 합니다. 예를 들어 `.text([text])` 대신 `.getText()`와 `.setText(text)`
형식으로 함수를 설계하면 됩니다. 포럼에 이 문제에 대한
[논의](https://github.com/electron/electron/issues/46)가
진행되고 있습니다.

# 코딩 스타일

이 가이드는 Electron의 코딩 스타일에 관해 설명합니다.

## C++과 Python

C++과 Python 스크립트는 Chromium의 [코딩 스타일](http://www.chromium.org/developers/coding-style)을 따릅니다.
파이선 스크립트 `script/cpplint.py`를 사용하여 모든 파일이 해당 코딩스타일에 맞게 코딩 되었는지 확인할 수 있습니다.

Python 버전은 2.7을 사용합니다.

C++ 코드는 많은 Chromium의 추상화와 타입을 사용합니다. 따라서 Chromium 코드에 대해 잘 알고 있어야 합니다.
이와 관련하여 시작하기 좋은 장소로 Chromium의 [Important Abstractions and Data Structures]
(https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures) 문서가 있습니다.
이 문서에선 몇가지 특별한 타입과 스코프 타입(스코프 밖으로 나가면 자동으로 메모리에서 할당을 해제합니다. 스마트 포인터로 보시면 됩니다)
그리고 로깅 메커니즘 등을 언급하고 있습니다.

## CoffeeScript

CoffeeScript의 경우 GitHub의 [스타일 가이드](https://github.com/styleguide/javascript)를 기본으로 따릅니다.
그리고 다음 규칙을 따릅니다:

* Google의 코딩 스타일에도 맞추기 위해 파일의 끝에는 **절대** 개행을 삽입해선 안됩니다.
* 파일 이름의 공백은 `_`대신에 `-`을 사용하여야 합니다. 예를 들어 `file_name.coffee`를
`file-name.coffee`로 고쳐야합니다. 왜냐하면 [github/atom](https://github.com/github/atom) 에서 사용되는 모듈의 이름은
보통 `module-name`형식이기 때문입니다. 이 규칙은 '.coffee' 파일에만 적용됩니다.

## API 이름

새로운 API를 만들 땐 getter, setter스타일 대신 jQuery의 one-function스타일을 사용해야 합니다. 예를 들어
`.getText()`와 `.setText(text)`대신에 `.text([text])`형식으로 설계하면 됩니다.
포럼에 이 문제에 대한 [논의](https://github.com/atom/electron/issues/46)가 있습니다.

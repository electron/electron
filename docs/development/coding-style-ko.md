# 코딩 스타일

## C++과 Python

C++과 Python스크립트는 Chromium의 [코딩 스타일](http://www.chromium.org/developers/coding-style)을 따릅니다.
파이선 스크립트 `script/cpplint.py`를 사용하여 모든 파일이 해당 코딩스타일에 맞게 코딩 되었는지 확인할 수 있습니다.

파이선의 버전은 2.7을 사용합니다.

## CoffeeScript

CoffeeScript의 경우 GitHub의 [스타일 가이드](https://github.com/styleguide/javascript)를 따릅니다, 동시에 다음 규칙도 따릅니다:

* Google의 코딩 스타일에도 맞추기 위해, 파일의 끝에는 **절대** 개행을 삽입해선 안됩니다.
* 파일 이름의 공백은 `_`대신에 `-`을 사용하여야 합니다. 예를 들어 `file_name.coffee`를
`file-name.coffee`로 고쳐야합니다, 왜냐하면 [github/atom](https://github.com/github/atom) 에서 사용되는 모듈의 이름은
보통 `module-name`형식이기 때문입니다, 이 규칙은 '.coffee' 파일에만 적용됩니다.

## API 이름

새로운 API를 만들 땐, getter, setter스타일 대신 jQuery의 one-function스타일을 사용해야 합니다. 예를 들어,
`.getText()`와 `.setText(text)`대신에 `.text([text])`형식으로 설계하면 됩니다.
포럼에 이 문제에 대한 [논의](https://github.com/atom/electron/issues/46)가 있습니다.

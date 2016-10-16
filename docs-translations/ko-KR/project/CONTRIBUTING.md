# Electron에 기여하기

:+1::tada: 먼저, 기여에 관심을 가져주셔서 감사합니다! :tada::+1:

이 프로젝트는 기여자 규약인 [행동강령](CODE_OF_CONDUCT.md)을 준수합니다. 따라서 이
프로젝트의 개발에 참여하려면 이 규약을 지켜야 합니다. 받아들일 수 없는 행위를 발견했을
경우 atom@github.com로 보고하세요.

다음 항목들은 Electron에 기여하는 가이드라인을 제시합니다.
참고로 이 항목들은 그저 가이드라인에 불과하며 규칙이 아닙니다. 따라서 스스로의 적절한
판단에 따라 이 문서의 변경을 제안할 수 있으며 변경시 pull request를 넣으면 됩니다.

## 이슈 제출

* [여기](https://github.com/electron/electron/issues/new)에서 새로운 이슈를 만들 수
있습니다. 하지만 이슈를 작성하기 전에 아래의 항목들을 숙지하고 가능한한 이슈 보고에
대해 최대한 많은 정보와 자세한 설명을 포함해야 합니다. 가능하다면 다음 항목을 포함해야
합니다:
  * 사용하고 있는 Electron의 버전
  * 현재 사용중인 운영체제
  * 가능하다면 무엇을 하려고 했고, 어떤 결과를 예측했으며, 어떤 것이 예측된대로
  작동하지 않았는지에 대해 서술해야 합니다.
* 추가로 다음 사항을 준수하면 이슈를 해결하는데 큰 도움이 됩니다:
  * 스크린샷 또는 GIF 애니메이션 이미지들
  * 터미널에 출력된 에러의 내용 또는 개발자 도구, 알림창에 뜬 내용
  * [Cursory search](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)를
  통해 이미 비슷한 내용의 이슈가 등록되어있는지 확인

## Pull Request 하기

* 가능한한 스크린샷과 GIF 애니메이션 이미지를 pull request에 추가
* JavaScript, C++과 Python등
[참조 문서에 정의된 코딩스타일](/docs-translations/ko-KR/development/coding-style.md)을
준수
* [문서 스타일 가이드](/docs-translations/ko-KR/styleguide.md)에 따라 문서를
[Markdown](https://daringfireball.net/projects/markdown) 형식으로 작성.
* 짧은, 현재 시제 커밋 메시지 사용. [커밋 메시지 스타일 가이드](#git-커밋-메시지)를
참고하세요

## 스타일 가이드

### 공통 코드

* 파일 마지막에 공백 라인(newline) 추가
* 다음 순서에 맞춰서 require 코드 작성:
  * Node 빌트인 모듈 (`path` 같은)
  * Electron 모듈 (`ipc`, `app` 같은)
  * 로컬 모듈 (상대 경로상에 있는)
* 다음 순서에 맞춰서 클래스 속성 지정:
  * 클래스 메서드와 속성 (메서드는 `@`로 시작)
  * 인스턴스 메서드와 속성
* 플랫폼 종속적인 코드 자제:
  * 파일 이름 결합시 `path.join()`을 사용.
  * 임시 디렉터리가 필요할 땐 `/tmp` 대신 `os.tmpdir()`을 통해 접근.
* 명시적인 함수 종료가 필요할 땐 `return` 만 사용.
  * `return null`, `return undefined`, `null`, 또는 `undefined` 사용 X

### Git 커밋 메시지

* 현재 시제 사용 ("Added feature" 대신 "Add feature" 사용)
* 명령법(imperative mood) 사용 ("Moves cursor to..." 대신 "Move cursor to..." 사용)
* 첫 줄은 72자에 맞추거나 그 보다 적게 제한
* 자유롭게 필요에 따라 이슈나 PR링크를 참조
* 단순한 문서 변경일 경우 `[ci skip]`을 커밋 메시지에 추가
* 커밋 메시지의 도입부에 의미있는 이모티콘 사용:
  * :art: `:art:` 코드의 포맷이나 구조를 개선(추가)했을 때
  * :racehorse: `:racehorse:` 성능을 개선했을 때
  * :non-potable_water: `:non-potable_water:` 메모리 누수를 연결했을 때
  * :memo: `:memo:` 문서를 작성했을 때
  * :penguin: `:penguin:` Linux에 대한 패치를 했을 때
  * :apple: `:apple:` macOS에 대한 패치를 했을 때
  * :checkered_flag: `:checkered_flag:` Windows에 대한 패치를 했을 때
  * :bug: `:bug:` 버그를 고쳤을 때
  * :fire: `:fire:` 코드 또는 파일을 삭제했을 때
  * :green_heart: `:green_heart:` CI 빌드를 고쳤을 때
  * :white_check_mark: `:white_check_mark:` 테스트를 추가했을 때
  * :lock: `:lock:` 보안 문제를 해결했을 때
  * :arrow_up: `:arrow_up:` 의존성 라이브러리를 업데이트 했을 때
  * :arrow_down: `:arrow_down:` 의존성 라이브러리를 다운그레이드 했을 때
  * :shirt: `:shirt:` linter(코드 검사기)의 경고를 제거했을 때

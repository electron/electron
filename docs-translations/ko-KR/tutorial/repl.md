# REPL

Read-Eval-Print-Loop (REPL) 은 단일 사용자 입력 (즉, 하나의 표현) 을 받아서,
평가하고, 사용자에게 결과를 반환하는 간단한 대화형 컴퓨터 프로그래밍 환경입니다.

`repl` 모듈은 REPL 구현을 제공하며 다음과 같이 접근할 수 있습니다:

* `electron` 또는 `electron-prebuilt` 지역 프로젝트 의존성으로 설치했다고
  가정하면:

  ```sh
  ./node_modules/.bin/electron --interactive
  ```
* `electron` 또는 `electron-prebuilt` 를 전역으로 설치했다고 가정하면:

  ```sh
  electron --interactive
  ```

이것은 메인 프로세스에 대한 REPL 만 생성합니다. 렌더러 프로세스를 위한 REPL 을
얻기 위해 개발자 도구의 콘솔 탭을 사용할 수 있습니다.

**참고:** `electron --interactive` 는 Windows 에서 사용할 수 없습니다.

자세한 정보는 [Node.js REPL 문서](https://nodejs.org/dist/latest/docs/api/repl.html)를
참고하세요.

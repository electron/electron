# Task Object

* `program` String - 실행할 프로그램의 경로.
  보통 현재 작동중인 애플리케이션의 경로인 `process.execPath`를 지정합니다.
* `arguments` String - `program`이 실행될 때 사용될 명령줄 인수.
* `title` String - JumpList에 표시할 문자열.
* `description` String - 이 작업에 대한 설명.
* `iconPath` String - JumpList에 표시될 아이콘의 절대 경로. 아이콘을 포함하고
  있는 임의의 리소스 파일을 사용할 수 있습니다. 보통 애플리케이션의 아이콘을
  그대로 사용하기 위해 `process.execPath`를 지정합니다.
* `iconIndex` Integer - 아이콘 파일의 인덱스. 만약 아이콘 파일이 두 개 이상의
  아이콘을 가지고 있을 경우, 사용할 아이콘의 인덱스를 이 옵션으로 지정해 주어야
  합니다. 단, 아이콘을 하나만 포함하고 있는 경우 0을 지정하면 됩니다.

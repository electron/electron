# ShortcutDetails Object

* `target` String - 이 바로가기로부터 실행될 대상입니다.
* `cwd` String (optional) - 작업 디렉토리입니다. 기본값은 없습니다.
* `args` String (optional) - 이 바로가기로부터 실행될 때 `target`에 적용될 인수
  값입니다. 기본값은 없습니다.
* `description` String (optional) - 바로가기의 설명입니다. 기본값은 없습니다.
* `icon` String (optional) - 아이콘의 경로입니다. DLL 또는 EXE가 될 수 있습니다.
  `icon`과 `iconIndex`는 항상 같이 설정되어야 합니다. 기본값은 없으며 `target`의
  아이콘을 사용합니다.
* `iconIndex` Integer (optional) - `icon`이 DLL 또는 EXE일 때 사용되는 아이콘의
  리소스 ID이며 기본값은 0입니다.
* `appUserModelId` String (optional) - 애플리케이션 사용자 모델 ID입니다.
  기본값은 없습니다.

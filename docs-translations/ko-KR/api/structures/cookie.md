# Cookie Object

* `name` String - 쿠키의 이름.
* `value` String - 쿠키의 값.
* `domain` String - 쿠키의 도메인.
* `hostOnly` String - 쿠키가 호스트 전용인가에 대한 여부.
* `path` String - 쿠키의 경로.
* `secure` Boolean - 쿠키가 안전한 것으로 표시됐는지에 대한 여부.
* `httpOnly` Boolean - 쿠키가 HTTP 전용으로 표시됐는지에 대한 여부.
* `session` Boolean - 쿠키가 세션 쿠키인지 만료일이 있는 영구 쿠키인지에 대한
  여부.
* `expirationDate` Double - (Optional) UNIX 시간으로 표시되는 쿠키의 만료일에
  대한 초 단위 시간. 세션 쿠키는 지원되지 않음.

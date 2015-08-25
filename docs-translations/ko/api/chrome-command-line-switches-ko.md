# 크롬 Command-Line 스위치 지원

다음 Command-Line 스위치들은 크롬 브라우저에서 제공되는 추가 옵션이며 Electron에서도 지원합니다.
[app][app]의 [ready][ready]이벤트가 작동하기 전에 [app.commandLine.appendSwitch][append-switch] API를 사용하면
어플리케이션 내부에서 스위치들을 추가할 수 있습니다:

```javascript
var app = require('app');
app.commandLine.appendSwitch('remote-debugging-port', '8315');
app.commandLine.appendSwitch('host-rules', 'MAP * 127.0.0.1');

app.on('ready', function() {
  // Your code here
});
```

## --client-certificate=`path`

`path`를 클라이언트 인증서로 설정합니다.

## --ignore-connections-limit=`domains`

`domains` 리스트(`,`로 구분)의 연결 제한을 무시합니다.

## --disable-http-cache

HTTP 요청 캐시를 비활성화 합니다.

## --remote-debugging-port=`port`

지정한 `port`에 HTTP기반의 리모트 디버거를 활성화 시킵니다. (개발자 콘솔)

## --proxy-server=`address:port`

시스템 설정의 프록시 서버를 무시하고 지정한 서버로 연결합니다. HTTP와 HTTPS 요청에만 적용됩니다.

## --proxy-pac-url=`url`

지정한 `url`의 PAC 스크립트를 사용합니다.

## --no-proxy-server

프록시 서버를 사용하지 않습니다. 다른 프록시 서버 플래그 및 설정을 무시하고 언제나 직접 연결을 사용합니다.

## --host-rules=`rules`

Hostname 맵핑 규칙을 설정합니다. (`,`로 분리)

예시:

* `MAP * 127.0.0.1` Forces all hostnames to be mapped to 127.0.0.1
* `MAP *.google.com proxy` Forces all google.com subdomains to be resolved to
  "proxy".
* `MAP test.com [::1]:77` Forces "test.com" to resolve to IPv6 loopback. Will
  also force the port of the resulting socket address to be 77.
* `MAP * baz, EXCLUDE www.google.com` Remaps everything to "baz", except for
  "www.google.com".
  
이 맵핑은 네트워크 요청시의 endpoint를 지정합니다. (TCP 연결과 직접 연결의 호스트 resolver, http 프록시 연결의 `CONNECT`, `SOCKS` 프록시 연결의 endpoint 호스트)

## --host-resolver-rules=`rules`

`--host-rules` 플래그와 비슷하지만 이 플래그는 host resolver에만 적용됩니다.

[app]: app-ko.md
[append-switch]: app-ko.md#appcommandlineappendswitchswitch-value
[ready]: app-ko.md#event-ready

## --ignore-certificate-errors

인증서 에러를 무시합니다.

## --ppapi-flash-path=`path`

Pepper 플래시 플러그인의 위치를 설정합니다.

## --ppapi-flash-version=`version`

Pepper 플래시 플러그인의 버전을 설정합니다.

## --log-net-log=`path`

Net log 이벤트를 지정한 `path`에 로그로 기록합니다.

## --v=`log_level`

기본 V-logging 최대 활성화 레벨을 지정합니다. 기본값은 0입니다. 기본적으로 양수를 레벨로 사용합니다.

`--v=-1`를 사용하면 로깅이 비활성화 됩니다.

## --vmodule=`pattern`

`--v` 옵션에 전달된 값을 덮어쓰고 모듈당 최대 V-logging 레벨을 지정합니다.
예를 들어 `my_module=2,foo*=3`는 `my_module.*`, `foo*.*`와 같은 파일 이름 패턴을 가진 모든 소스 코드들의 로깅 레벨을 각각 2와 3으로 설정합니다.

슬래시(`/`), 백슬래시(`\`)를 포함하는 모든 패턴은 모듈뿐만 아니라 모든 경로명에 대해서도 테스트 됩니다.
예를 들어 `*/foo/bar/*=2` 표현식은 `foo/bar` 디렉터리 안의 모든 소스 코드의 로깅 레벨을 2로 지정합니다.

모든 크로미움과 관련된 로그를 비활성화하고 어플리케이션의 로그만 활성화 하려면 다음과 같이 코드를 작성하면 됩니다:


```javascript
app.commandLine.appendSwitch('v', -1);
app.commandLine.appendSwitch('vmodule', 'console=0');
```

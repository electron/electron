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

These mappings apply to the endpoint host in a net request (the TCP connect
and host resolver in a direct connection, and the `CONNECT` in an http proxy
connection, and the endpoint host in a `SOCKS` proxy connection).

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

Gives the per-module maximal V-logging levels to override the value given by
`--v`. E.g. `my_module=2,foo*=3` would change the logging level for all code in
source files `my_module.*` and `foo*.*`.

Any pattern containing a forward or backward slash will be tested against the
whole pathname and not just the module. E.g. `*/foo/bar/*=2` would change the
logging level for all code in source files under a `foo/bar` directory.

To disable all chromium related logs and only enable your application logs you
can do:

```javascript
app.commandLine.appendSwitch('v', -1);
app.commandLine.appendSwitch('vmodule', 'console=0');
```

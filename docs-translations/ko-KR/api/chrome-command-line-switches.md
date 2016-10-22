# 크롬 명령줄 스위치 지원

> Electron에서 지원하는 커맨드 명령줄 스위치입니다.

애플리케이션 메인 스크립트의 [app][app] 모듈에서 [ready][ready] 이벤트가 실행되기
전에 [app.commandLine.appendSwitch][append-switch]를 호출하면, 애플리케이션의
명령줄 옵션을 추가로 지정할 수 있습니다:

```javascript
const {app} = require('electron')
app.commandLine.appendSwitch('remote-debugging-port', '8315')
app.commandLine.appendSwitch('host-rules', 'MAP * 127.0.0.1')

app.on('ready', () => {
  // Your code here
})
```

## --ignore-connections-limit=`domains`

`domains` 리스트(`,`로 구분)의 연결 제한을 무시합니다.

## --disable-http-cache

HTTP 요청 캐시를 비활성화합니다.

## --disable-http2

HTTP/2와 SPDY/3.1 프로토콜을 비활성화합니다.

## --debug=`port` and --debug-brk=`port`

디버깅관련 플래그입니다. 자세한 내용은 [메인프로세스 디버깅하기]
[debugging-main-process] 안내서를 보세요.

## --remote-debugging-port=`port`

지정한 `port`에 HTTP 기반의 리모트 디버거를 활성화합니다. (개발자 도구)

## --js-flags=`flags`

Node JS 엔진에 지정한 플래그를 전달합니다. `flags`를 메인 프로세스에서
활성화하고자 한다면, Electron이 시작되기 전에 스위치를 전달해야 합니다.

```bash
$ electron --js-flags="--harmony_proxies --harmony_collections" your-app
```

가능한 플래그 목록은 [Node 문서][node-cli]를 보거나 터미널에서 `node --help`
명령을 실행하세요. 또한, 구체적으로 노드의 V8 자바스크립트 엔진과 관련있는
플래그의 목록을 보려면 `node --v8-options` 를 실행하세요.

## --proxy-server=`address:port`

시스템 설정의 프록시 서버를 무시하고 지정한 서버로 연결합니다. HTTP와 HTTPS
요청에만 적용됩니다.

시스템 프록시 서버 설정을 무시하고 지정한 서버로 연결합니다. 이 스위치는 HTTP와
HTTPS 그리고 WebSocket 요청에만 적용됩니다. 그리고 모든 프록시 서버가 HTTPS가
WebSocket 요청을 지원하지 않고 있을 수 있으므로 사용시 주의해야 합니다.

## --proxy-bypass-list=`hosts`

Electron이 세미콜론으로 구분된 호스트 리스트에서 지정한 프록시 서버를 건너뛰도록
지시합니다.

예시:

```javascript
app.commandLine.appendSwitch('proxy-bypass-list', '<local>;*.google.com;*foo.com;1.2.3.4:5678')
```

위 예시는 로컬 주소(`localhost`, `127.0.0.1`, 등)와 `google.com`의 서브도메인,
`foo.com`을 접미사로 가지는 호스트, `1.2.3.4:5678` 호스트를 제외한 모든 연결에서
프록시 서버를 사용합니다.

## --proxy-pac-url=`url`

지정한 `url`의 PAC 스크립트를 사용합니다.

## --no-proxy-server

프록시 서버를 사용하지 않습니다. 다른 프록시 서버 플래그 및 설정을 무시하고
언제나 직접 연결을 사용합니다.

## --host-rules=`rules`

Hostname 맵핑 규칙을 설정합니다. (`,`로 구분)

예시:

* `MAP * 127.0.0.1` 강제적으로 모든 호스트네임을 127.0.0.1로 맵핑합니다
* `MAP *.google.com proxy` 강제적으로 모든 google.com의 서브도메인을 "proxy"로
  연결합니다
* `MAP test.com [::1]:77` 강제적으로 "test.com"을 IPv6 루프백으로 연결합니다.
  소켓 주소의 포트 또한 77로 고정됩니다.
* `MAP * baz, EXCLUDE www.google.com` "www.google.com"을 제외한 모든 것들을
  "baz"로 맵핑합니다.

이 맵핑은 네트워크 요청시의 endpoint를 지정합니다. (TCP 연결과 직접 연결의 호스트
resolver, http 프록시 연결의 `CONNECT`, `SOCKS` 프록시 연결의 endpoint 호스트)

## --host-resolver-rules=`rules`

`--host-rules` 플래그와 비슷하지만 이 플래그는 host resolver에만 적용됩니다.

## --auth-server-whitelist=`url`

통합 인증을 사용하도록 설정할 쉼표로 구분된 서버의 리스트.

예를 들어:

```
--auth-server-whitelist='*example.com, *foobar.com, *baz'
```

그리고 모든 `example.com`, `foobar.com`, `baz`로 끝나는 `url`은 통합 인증을
사용하도록 설정됩니다. `*` 접두어가 없다면 url은 정확히 일치해야 합니다.

## --auth-negotiate-delegate-whitelist=`url`

필수적인 사용자 자격 증명을 보내야 할 쉼표로 구분된 서버의 리스트.
`*` 접두어가 없다면 url은 정확히 일치해야 합니다.

## --ignore-certificate-errors

인증서 에러를 무시합니다.

## --ppapi-flash-path=`path`

Pepper 플래시 플러그인의 위치를 설정합니다.

## --ppapi-flash-version=`version`

Pepper 플래시 플러그인의 버전을 설정합니다.

## --log-net-log=`path`

Net log 이벤트를 활성화하고 `path`에 로그를 기록합니다.

## --ssl-version-fallback-min=`version`

TLS fallback에서 사용할 SSL/TLS 최소 버전을 지정합니다. (`tls1`, `tls1.1`, `tls1.2`)

## --cipher-suite-blacklist=`cipher_suites`

SSL 암호화를 비활성화할 대상 목록을 지정합니다. (`,`로 구분)

## --disable-renderer-backgrounding

Chromium이 렌더러 프로세스의 보이지 않는 페이지의 우선순위를 낮추는 것을 방지합니다.

이 플래그는 전역적이며 모든 렌더러 프로세스에 적용됩니다. 만약 하나의 윈도우
창에만 스로틀링을 비활성화하고 싶다면 [조용한 오디오를 재생하는][play-silent-audio]
핵을 사용할 수 있습니다.

## --enable-logging

Chromium의 로그를 콘솔에 출력합니다.

이 스위치는 애플리케이션이 로드되기 전에 분석 되므로 `app.commandLine.appendSwitch`
메서드에선 사용할 수 없습니다. 하지만 `ELECTRON_ENABLE_LOGGING` 환경 변수를
설정하면 본 스위치를 지정한 것과 같은 효과를 낼 수 있습니다.

## --v=`log_level`

기본 V-logging 최대 활성화 레벨을 지정합니다. 기본값은 0입니다. 기본적으로 양수를
레벨로 사용합니다.

이 스위치는 `--enable-logging` 스위치를 같이 지정해야 작동합니다.

## --vmodule=`pattern`

`--v` 옵션에 전달된 값을 덮어쓰고 모듈당 최대 V-logging 레벨을 지정합니다.
예를 들어 `my_module=2,foo*=3`는 `my_module.*`, `foo*.*`와 같은 파일 이름 패턴을
가진 모든 소스 코드들의 로깅 레벨을 각각 2와 3으로 설정합니다.

또한 슬래시(`/`) 또는 백슬래시(`\`)를 포함하는 패턴은 지정한 경로에 대해 패턴을
테스트 합니다. 예를 들어 `*/foo/bar/*=2` 표현식은 `foo/bar` 디렉터리 안의 모든
소스 코드의 로깅 레벨을 2로 지정합니다.

이 스위치는 `--enable-logging` 스위치를 같이 지정해야 작동합니다.

[app]: app.md
[append-switch]: app.md#appcommandlineappendswitchswitch-value
[ready]: app.md#event-ready
[play-silent-audio]: https://github.com/atom/atom/pull/9485/files
[debugging-main-process]: ../tutorial/debugging-main-process.md
[node-cli]: https://nodejs.org/api/cli.html

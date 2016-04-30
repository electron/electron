# 디버거에서 디버그 심볼 서버 설정

디버그 심볼은 디버깅 세션을 더 좋게 개선해 줍니다. 디버그 심볼은 실행 파일과 동적 링크
라이브러리에서 메서드에 대한 정보를 담고 있으며 명료한 함수 호출 스텍 정보를 제공합니다.
심볼 서버는 유저가 크기가 큰 디버깅용 파일을 필수적으로 다운로드 받지 않고도 디버거가
알맞은 심볼, 바이너리 그리고 소스를 자동적으로 로드할 수 있도록 해줍니다. 서버 사용법은
[Microsoft의 심볼 서버](http://support.microsoft.com/kb/311503)와 비슷합니다.
사용법을 알아보려면 이 문서를 참조하세요.

참고로 릴리즈된 Electron 빌드는 자체적으로 많은 최적화가 되어 있는 관계로 경우에 따라
디버깅이 쉽지 않을 수 있습니다. Inlining, tail call 등 컴파일러 최적화에 의해 디버거가
모든 변수의 콘텐츠를 보여줄 수 없는 경우도 있고 실행 경로가 이상하게 보여지는 경우도
있습니다. 유일한 해결 방법은 최적화되지 않은 로컬 빌드를 하는 것입니다.

공식적인 Electron의 심볼 서버의 URL은
http://54.249.141.255:8086/atom-shell/symbols 입니다. 일단 이 URL에 직접적으로
접근할 수는 없습니다: 디버깅 툴에 심볼의 경로를 추가해야 합니다. 아래의 예제를 참고하면
로컬 캐시 디렉터리는 서버로부터 중복되지 않게 PDB를 가져오는데 사용됩니다.
`c:\code\symbols` 캐시 디렉터리를 사용중인 OS에 맞춰 적당한 경로로 변경하세요.

## Windbg에서 심볼 서버 사용하기

Windbg 심볼 경로는 구분자와 `*` 문자로 설정되어 있습니다. Electron 심볼 서버만
사용하려면 심볼 경로의 엔트리를 추가해야 합니다. (**참고:**  `c:\code\symbols`
디렉터리 경로를 PC가 원하는 경로로 수정할 수 있습니다):

```
SRV*c:\code\symbols\*http://54.249.141.255:8086/atom-shell/symbols
```

Windbg 메뉴 또는 `.sympath` 커맨드를 이용하여 환경에 `_NT_SYMBOL_PATH` 문자열을
설정합니다. 만약 Microsoft의 심볼서버로부터 심볼을 받아오려면 다음과 같이 리스팅을
먼저해야 합니다:

```
SRV*c:\code\symbols\*http://msdl.microsoft.com/download/symbols;SRV*c:\code\symbols\*http://54.249.141.255:8086/atom-shell/symbols
```

## Visual Studio에서 심볼 서버 사용하기

<img src='http://mdn.mozillademos.org/files/733/symbol-server-vc8express-menu.jpg'>
<img src='http://mdn.mozillademos.org/files/2497/2005_options.gif'>

## 문제 해결: Symbols will not load

Windbg에서 다음의 커맨드를 입력하여 왜 심볼이 로드되지 않았는지에 대한 오류 내역을
출력합니다:

```
> !sym noisy
> .reload /f chromiumcontent.dll
```

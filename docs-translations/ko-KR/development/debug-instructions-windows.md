# Windows에서 Electron 디버깅하기

만약 작성한 Javascript 어플리케이션이 아닌 Electron 자체의 크래시나 문제를 경험하고
있다면, 네이티브/C++ 디버깅에 익숙하지 않은 개발자는 디버깅이 약간 까다로울 수
있습니다. 그렇다 해도, Visual Studio, GitHub의 Electron이 호스팅하는 심볼 서버,
Electron 소스 코드가 중단점을 통해 순차적으로 쉽게 디버깅할 수 있는 환경을 제공합니다.

## 요구 사항

* **Electron의 디버그 빌드**: 가장 쉬운 방법은 보통
  [Windows용 빌드 설명서](build-instructions-windows.md)에 명시된 요구사항과 툴을
  사용하여 스스로 빌드하는 것입니다. 물론 직접 다운로드 받은 Electron 바이너리에도
  디버거 연결 및 디버깅을 사용할 수 있지만, 실질적으로 디버깅이 까다롭게 고도의
  최적화가 되어있음을 발견하게 될 것입니다: 인라인화, 꼬리 호출, 이외 여러 가지
  생소한 최적화가 적용되어 디버거가 모든 변수와 실행 경로를 정상적으로 표시할 수
  없습니다.

* **Visual Studio와 C++ 툴**: Visual Studio 2013과 Visual Studio 2015 두 가지
  커뮤니티 에디션 모두 잘 작동합니다. 설치가 완료되면,
  [Visual Studio가 GitHub의 Electron 심볼 서버를 사용하도록](setting-up-symbol-server.md)
  설정해야 합니다. 이 작업은 Visual Studio가 Electron에서 무슨일이 일어나는지 더 잘
  이해할 수 있도록 하며 변수를 사람이 읽기 좋은 포맷으로 쉽게 표현할 수 있도록 합니다.

* **ProcMon**: 이 무료 [SysInternals][sys-internals] 툴은 프로세스 인자, 파일
  핸들러 그리고 레지스트리 작업을 탐색할 수 있게 도와줍니다.

## Electron에 디버거 연결하고 디버깅하기

디버깅 작업을 시작하려면, PowerShell/CMD 중 한 가지를 열고 디버그 빌드 상태의
Electron에 인자로 어플리케이션을 전달하여 실행합니다:

```powershell
$ ./out/D/electron.exe ~/my-electron-app/
```

### 중단점 설정

그리고, Visual Studio를 엽니다. Electron은 Visual Studio로 만들어지지 않았으며
이러한 이유로 인해 프로젝트 파일을 가지고 있지 않습니다. 하지만 "파일로 열기"를 통해
소스 코드 파일들을 열 수 있습니다. Visual Studio가 각각의 파일을 따로 연다는 것입니다.
여전히 중단점을 설정할 수 있습니다. Visual Studio는 현재 소스 코드와 일치하는 작동
중인 프로세스와 중단점을 자동으로 찾아냅니다.

관련된 코드 파일들은 `./atom/`에서 찾을 수 있으며 또한 Brightray 안
`./vendor/brightray/browser`와 `./vendor/brightray/common`에서도 찾을 수 있습니다.
만약 하드코어를 좋아한다면, Chromium을 직접 디버깅할 수도 있습니다. 확실히
`chromium_src` 안에서 찾을 수 있습니다.

### 디버거 연결

로컬에서 작동 중인 프로세스 또는 원격 컴퓨터에 Visual Studio 디버거를 적용시킬 수
있습니다. 프로세스의 실행이 끝난 후, 디버그 / 프로세스에 연결을 (또는 `Ctrl+Alt+P`
입력) 클릭하면 "프로세스에 연결" 대화 상자가 열립니다. 이 도구를 통해 로컬 또는
원격에서 작동 중인 어플리케이션을 디버깅할 수 있으며 여러 프로세스를 동시에 디버깅할
수도 있습니다.

만약 Electron이 서로 다른 유저 계정에서 실행 중이라면, `모든 사용자의 프로세스
보이기`를 선택하면 됩니다. 참고로 이는 `BrowserWindow`가 열린 개수에 따라 달라질 수
있으며 아마 다수의 프로세스를 발견할 수 있을 것입니다. 전형적인 one-window
어플리케이션은 Visual Studio에서 두 개의 `Electron.exe` 항목으로 표시됩니다. 하나는
메인 프로세스이며 다른 하나는 렌더러 프로세스입니다. 리스트는 단지 이름 하나만 제공하기
때문에 현재까지는 다른 적절한 프로세스 판별법이 없습니다.

## 어떤 프로세스에 디버거를 적용해야 하나요?

코드는 메인 프로세스 내에서 실행되며 (이는 코드를 안에서 찾을 수 있거나, 결국 메인
Javascript 파일에 의해 실행) remote (`require('electron').remote`)를 사용하여
코드를 실행하는 것 또한 메인 프로세스 내에서 실행됩니다. 다른 코드는 각각의 렌더러
프로세스 내에서 실행됩니다.

디버깅할 때 여러 프로그램에 디버거를 적용할 수 있지만, 언제나 한 개의 프로그램만
디버거에서 활성화되어야 합니다. `디버그 경로` 툴바 또는 `프로세스 창`에서 활성화
프로그램을 설정할 수 있습니다.

## 프로세스를 관찰하기 위해 ProcMon 사용

Visual Studio는 특정 코드 경로를 탐색하는것에 대해 환상적인 기능을 제공하고 ProcMon은
어플리케이션이 운영체제와 하는 일의 모든 것을 관찰하는데 강력한 기능을 가지고 있습니다.
이 툴은 프로세스의 파일, 레지스트리, 네트워킹, 프로세스, 프로파일링 상세를 포착할 수
있으며 강력하게 **모든** 이벤트의 발생을 로깅을 시도합니다. 만약 어플리케이션이
운영체제에 대해 무슨 일을 하고 있는지 이해하고 싶다면 이는 좋은 자원이 될 것입니다.

ProcMon의 기본적인 디버깅 기능을 알아보고 싶다면 Microsoft에서 제공하는
[동영상 강좌][procmon-instructions]를 참고하세요.

[sys-internals]: https://technet.microsoft.com/en-us/sysinternals/processmonitor.aspx
[procmon-instructions]: https://channel9.msdn.com/shows/defrag-tools/defrag-tools-4-process-monitor

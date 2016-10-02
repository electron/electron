# macOS 에서 디버깅하기

만약 작성한 Javascript 애플리케이션이 아닌 Electron 자체의 크래시나 문제를
경험하고 있다면, 네이티브/C++ 디버깅에 익숙하지 않은 개발자는 디버깅이 약간
까다로울 수 있습니다. 그렇다 해도, lldb, Electron 소스 코드가 중단점을 통해
순차적으로 쉽게 디버깅할 수 있느 환경을 제공합니다.

## 요구사항

* **Electron의 디버그 빌드**: 가장 쉬운 방법은 보통 [macOS용 빌드 설명서]
  (buildinstruction-osx.md)에 명시된 요구 사항과 툴을 사용하여 스스로 빌드하는
  것입니다. 물론 직접 다운로드 받은 Electron 바이너리에도 디버거 연결 및
  디버깅을 사용할 수 있지만, 실질적으로 디버깅이 까다롭게 고도의 최적화가
  되어있음을 발견하게 될 것입니다: 인라인화, 꼬리 호출, 이외 여러 가지 생소한
  최적화가 적용되어 디버거가 모든 변수와 실행 경로를 정상적으로 표시할 수
  없습니다.

* **Xcode**: Xcode 뿐만 아니라, Xcode 명령 줄 도구를 설치합니다. 이것은 LLDB,
  macOS Xcode 의 기본 디버거를 포함합니다. 그것은 데스크톱과 iOS 기기와
  시뮬레이터에서 C, Objective-C, C++ 디버깅을 지원합니다.

## Electron에 디버거 연결하고 디버깅하기

디버깅 작업을 시작하려면, Terminal 을 열고 디버그 빌드 상태의 Electron 을
전달하여 `lldb` 를 시작합니다.


```bash
$ lldb ./out/D/Electron.app
(lldb) target create "./out/D/Electron.app"
Current executable set to './out/D/Electron.app' (x86_64).
```

### 중단점 설정

LLDB 는 강력한 도구이며 코드 검사를 위한 다양한 전략을 제공합니다. 간단히
소개하자면, JavaScript 에서 올바르지 않은 명령을 호출한다고 가정합시다 - 당신은
명령의 C++ 부분에서 멈추길 원하며 그것은 Electron 소스 내에 있습니다.

관련 코드는 `./atom/` 에서 찾을 수 있으며 마찬가지로 Brightray 도
`./vendor/brightray/browser` 와 `./vendor/brightray/common` 에서 찾을 수
있습니다. 당신이 열정적이라면, Chromium 을 직접 디버깅할 수 있으며,
`chromium_src` 에서 찾을 수 있습니다.

app.setName() 을 디버깅한다고 가정합시다, 이것은 browser.cc 에
Browser::SetName() 으로 정의되어있습니다. breakpoint 명령으로 멀추려는 파일과
줄을 명시하여 중단점을 설정합시다:

```bash
(lldb) breakpoint set --file browser.cc --line 117
Breakpoint 1: where = Electron Framework``atom::Browser::SetName(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&) + 20 at browser.cc:118, address = 0x000000000015fdb4
```

그리고 Electron 을 시작하세요:

```bash
(lldb) run
```

Electron 이 시작시에 앱의 이름을 설정하기때문에, 앱은 즉시 중지됩니다:

```bash
(lldb) run
Process 25244 launched: '/Users/fr/Code/electron/out/D/Electron.app/Contents/MacOS/Electron' (x86_64)
Process 25244 stopped
* thread #1: tid = 0x839a4c, 0x0000000100162db4 Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 20 at browser.cc:118, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
    frame #0: 0x0000000100162db4 Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 20 at browser.cc:118
   115 	}
   116
   117 	void Browser::SetName(const std::string& name) {
-> 118 	  name_override_ = name;
   119 	}
   120
   121 	int Browser::GetBadgeCount() {
(lldb)
```

현재 매개변수와 지역 변수를 보기위해, `frame variable` (또는 `fr v`) 를
실행하면, 현재 앱 이름이 "Electron" 인 것을 불 수 있습니다.

```bash
(lldb) frame variable
(atom::Browser *) this = 0x0000000108b14f20
(const string &) name = "Electron": {
    [...]
}
```

현재 선택된 쓰레드에서 소스 수준 한단계를 실행하기위해, `step` (또는 `s`) 를
실행하세요. `name_override_.empty()` 로 들어가게 됩니다. 스텝 오버 실행은,
`next` (또는 `n`) 을 실행하세요.

```bash
(lldb) step
Process 25244 stopped
* thread #1: tid = 0x839a4c, 0x0000000100162dcc Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 44 at browser.cc:119, queue = 'com.apple.main-thread', stop reason = step in
    frame #0: 0x0000000100162dcc Electron Framework`atom::Browser::SetName(this=0x0000000108b14f20, name="Electron") + 44 at browser.cc:119
   116
   117 	void Browser::SetName(const std::string& name) {
   118 	  name_override_ = name;
-> 119 	}
   120
   121 	int Browser::GetBadgeCount() {
   122 	  return badge_count_;
```

디버깅을 끝내려면, `process continue` 를 실행하세요. 또한 쓰레드에서 실행 줄
수를 지정할 수 있습니다 (`thread until 100`). 이 명령은 현재 프레임에서 100 줄에
도달하거나 현재 프레임을 나가려고 할 때 까지 쓰레드를 실행합니다.

이제, Electron 의 개발자 도구를 열고 `setName` 을 호출하면, 다시 중단점을 만날
것 입니다.

### 더 읽을거리
LLDB 는 훌륭한 문서가 있는 강력한 도구입니다. 더 학습하기 위해,
[LLDB 명령 구조 참고][lldb-command-structure] 와
[LLDB 를 독립 실행 디버거로 사용하기][lldb-standalone] 같은 애플의 디버깅
문서를 고려하세요.

LLDB 의 환상적인 [설명서와 학습서][lldb-tutorial] 를 확인할 수 있습니다.
그것은 더 복잡한 디버깅 시나리오를 설명합니다.

[lldb-command-structure]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/gdb_to_lldb_transition_guide/document/lldb-basics.html#//apple_ref/doc/uid/TP40012917-CH2-SW2
[lldb-standalone]: https://developer.apple.com/library/mac/documentation/IDEs/Conceptual/gdb_to_lldb_transition_guide/document/lldb-terminal-workflow-tutorial.html
[lldb-tutorial]: http://lldb.llvm.org/tutorial.html

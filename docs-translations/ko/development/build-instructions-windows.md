# 빌드 설명서 (Windows)

이 가이드는 Windows 운영체제에서 Electron을 빌드하는 방법을 설명합니다.

## 빌드전 요구 사항

* Windows 7 / Server 2008 R2 또는 최신 버전
* Visual Studio 2013 - [VS 2013 커뮤니티 에디션 무료 다운로드](http://www.visualstudio.com/products/visual-studio-community-vs)
* [Python 2.7](http://www.python.org/download/releases/2.7/)
* [Node.js](http://nodejs.org/download/)
* [Git](http://git-scm.com)

현재 사용하고 있는 PC에 Windows를 설치하지 않았다면 [modern.ie](https://www.modern.ie/en-us/virtualization-tools#downloads)에서
사용 기한이 정해져있는 무료 가상머신 버전의 Windows를 받아 Electron을 빌드하는 방법도 있습니다.

Electron은 모든 빌드를 command-line 스크립트를 통해 빌드합니다. 따라서 빌드에 Visual Studio를 사용할 수 없습니다.
하지만 여전히 Electron을 개발할 땐 아무 에디터나 사용할 수 있습니다. 빠른 시일내에 Visual Studio를 이용한 빌드도 지원할 계획입니다.

**참고:** Visual Studio가 직접 빌드에 사용되지 않더라도 IDE와 같이 제공된 빌드 툴체인이 빌드에 **필수적으로** 사용되므로 여전히 필요합니다.

**참고:** Visual Studio 2015는 사용할 수 없습니다. MSVS **2013**을 사용하고 있는지 확인해주세요.

## 코드 가져오기

```powershell
$ git clone https://github.com/atom/electron.git
```

## 부트 스트랩

부트스트랩 스크립트는 필수적인 빌드 종속성 라이브러리들을 모두 다운로드하고 프로젝트 파일을 생성합니다.
참고로 Electron은 빌드 툴체인으로 `ninja`를 사용하므로 Visual Studio 프로젝트는 생성되지 않습니다.

```powershell
$ cd electron
$ python script\bootstrap.py -v
```

## 빌드 하기

`Release` 와 `Debug` 두 타겟 모두 빌드 합니다:

```powershell
$ python script\build.py
```

또는 `Debug` 타겟만 빌드 할 수 있습니다:

```powershell
$ python script\build.py -c D
```

빌드가 모두 끝나면 `out/D` (디버그 타겟) 또는 `out/R` (릴리즈 타겟) 디렉터리에서 `electron.exe` 실행 파일을 찾을 수 있습니다.

## 64비트 빌드

64비트를 타겟으로 빌드 하려면 부트스트랩 스크립트를 실행할 때 `--target_arch=x64` 인자를 같이 넘겨주면 됩니다:

```powershell
$ python script\bootstrap.py -v --target_arch=x64
```

다른 빌드 단계도 정확하게 같습니다.

## 테스트

프로젝트 코딩 스타일을 확인하려면:

```powershell
$ python script\cpplint.py
```

테스트를 실행하려면:

```powershell
$ python script\test.py
```

테스트 실행시 `runas`와 같은 네이티브 모듈을 포함하는데 이 모듈은 디버그 빌드에서 같이 사용할 수 없습니다.
하지만 여전히 릴리즈 빌드에선 사용할 수 있습니다.

릴리즈 빌드로 테스트를 실행하려면 다음 커맨드를 사용하면 됩니다:

```powershell
$ python script\test.py -R
```

## 문제 해결

### Command xxxx not found

만약 `Command xxxx not found`와 같은 형식의 에러가 발생했다면 `VS2012 Command Prompt` 콘솔로 빌드 스크립트를 실행해볼 필요가 있습니다.

### Fatal internal compiler error: C1001

Visual Studio가 업데이트까지 완벽하게 설치된 최신버전인지 확인하세요.

### Assertion failed: ((handle))->activecnt >= 0

Cygwin에서 빌드 할 경우 `bootstrap.py` 스크립트가 다음의 에러와 함께 빌드에 실패할 수 있습니다:

```
Assertion failed: ((handle))->activecnt >= 0, file src\win\pipe.c, line 1430

Traceback (most recent call last):
  File "script/bootstrap.py", line 87, in <module>
    sys.exit(main())
  File "script/bootstrap.py", line 22, in main
    update_node_modules('.')
  File "script/bootstrap.py", line 56, in update_node_modules
    execute([NPM, 'install'])
  File "/home/zcbenz/codes/raven/script/lib/util.py", line 118, in execute
    raise e
subprocess.CalledProcessError: Command '['npm.cmd', 'install']' returned non-zero exit status 3
```

이 버그는 Cygwin Python과 Win32 Node를 같이 사용할 때 발생합니다.
부트스트랩 스크립트에서 Win32 Python을 사용함으로써 이 문제를 해결할 수 있습니다.
`C:\Python27` 디렉터리에 Python이 설치되었다는 가정하에 다음 명령을 실행하면 됩니다:

```bash
$ /cygdrive/c/Python27/python.exe script/bootstrap.py
```

### LNK1181: cannot open input file 'kernel32.lib'

32비트 Node.js를 다시 설치하세요.

### Error: ENOENT, stat 'C:\Users\USERNAME\AppData\Roaming\npm'

간단하게 해당 디렉터리를 생성하면 [문제가 해결될 겁니다](http://stackoverflow.com/a/25095327/102704):

```powershell
$ mkdir ~\AppData\Roaming\npm
```

### node-gyp is not recognized as an internal or external command

Git Bash로 빌드 했을 때 이러한 에러가 발생할 수 있습니다. 반드시 PowerShell이나 VS2012 Command Prompt에서 빌드를 진행해야 합니다.

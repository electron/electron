# 빌드 설명서 (macOS)

이 가이드는 macOS 운영체제에서 Electron을 빌드하는 방법을 설명합니다.

## 빌드전 요구 사항

* macOS >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (external)

만약 Homebrew를 이용해 파이선을 설치했다면 다음 Python 모듈도 같이 설치해야 합니다:

* pyobjc

## 코드 가져오기

```bash
$ git clone https://github.com/electron/electron.git
```

## 부트 스트랩

부트스트랩 스크립트는 필수적인 빌드 종속성 라이브러리들을 모두 다운로드하고 프로젝트
파일을 생성합니다. 참고로 Electron은 [ninja](https://ninja-build.org/)를 빌드
툴체인으로 사용하므로 Xcode 프로젝트는 생성되지 않습니다.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## 빌드하기

`Release` 와 `Debug` 두 타겟 모두 빌드 합니다:

```bash
$ ./script/build.py
```

또는 `Debug` 타겟만 빌드 할 수 있습니다:

```bash
$ ./script/build.py -c D
```

빌드가 모두 끝나면 `out/D` 디렉터리에서 `Electron.app` 실행 파일을 찾을 수 있습니다.

## 32비트 지원

Electron은 현재 macOS 64비트만 지원하고 있습니다. 그리고 앞으로도 macOS 32비트는 지원할
계획이 없습니다.

## 테스트

프로젝트 코딩 스타일을 확인하려면:

```bash
$ ./script/cpplint.py
```

테스트를 실행하려면:

```bash
$ ./script/test.py
```

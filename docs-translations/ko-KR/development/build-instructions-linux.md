# 빌드 설명서 (Linux)

이 가이드는 Linux 운영체제에서 Electron을 빌드하는 방법을 설명합니다.

## 빌드전 요구사양

* 최소한 25GB 이상의 디스크 공간과 8GB 램이 필요합니다.
* Python 2.7.x. 몇몇 CentOS와 같은 배포판들은 아직도 Python 2.6.x 버전을 사용합니다.
  그래서 먼저 `python -V`를 통해 버전을 확인할 필요가 있습니다.
* Node.js v0.12.x. Node를 설치하는 방법은 여러 가지가 있습니다. 먼저,
  [Node.js](http://nodejs.org) 사이트에서 소스 코드를 받아 빌드하는 방법입니다.
  이렇게 하면 Node를 일반 유저로 홈 디렉터리에 설치할 수 있습니다. 다른 방법으로는
  [NodeSource](https://nodesource.com/blog/nodejs-v012-iojs-and-the-nodesource-linux-repositories)에서
  소스 파일을 받아와 설치할 수 있습니다. 자세한 내용은 [Node.js 설치 방법](https://github.com/joyent/node/wiki/Installation)을
  참고하세요.
* Clang 3.4 또는 최신 버전
* GTK+ 와 libnotify의 개발용 헤더

Ubuntu를 사용하고 있다면 다음과 같이 라이브러리를 설치해야 합니다:

```bash
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 libnss3-dev gcc-multilib g++-multilib curl \
                       gperf bison
```

Fedora를 사용하고 있다면 다음과 같이 라이브러리를 설치해야 합니다:

```bash
$ sudo yum install clang dbus-devel gtk2-devel libnotify-devel libgnome-keyring-devel \
                   xorg-x11-server-utils libcap-devel cups-devel libXtst-devel \
                   alsa-lib-devel libXrandr-devel GConf2-devel nss-devel bison \
                   gperf
```

다른 배포판의 경우 pacman 같은 패키지 매니저를 통해 패키지를 설치 할 수 있습니다.
패키지의 이름은 대부분 위 예시와 비슷할 것입니다. 또는 소스 코드를 내려받아
직접 빌드하는 방법도 있습니다.

## 코드 가져오기

```bash
$ git clone https://github.com/electron/electron.git
```

## 부트 스트랩

부트스트랩 스크립트는 필수적인 빌드 종속성 라이브러리들을 모두 다운로드하고 프로젝트
파일을 생성합니다. 스크립트가 정상적으로 작동하기 위해선 Python 2.7.x 버전이
필요합니다. 아마 다운로드 작업이 상당히 많은 시간을 소요할 것입니다. 참고로 Electron은
`ninja`를 빌드 툴체인으로 사용하므로 `Makefile`은 생성되지 않습니다.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

### 크로스 컴파일

`arm` 아키텍쳐로 빌드 하려면 다음 종속성 라이브러리를 설치해야 합니다:

```bash
$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \
                       g++-arm-linux-gnueabihf
```

그리고 `bootstrap.py` 스크립트의 `--target_arch` 파라미터에 `arm` 또는 `ia32`
아키텍쳐를 지정하여 크로스 컴파일 할 수 있습니다:

```bash
$ ./script/bootstrap.py -v --target_arch=arm
```

## 빌드하기

`Release` 와 `Debug` 두 타겟 모두 빌드 합니다:

```bash
$ ./script/build.py
```

이 스크립트는 `out/R` 디렉터리에 크기가 매우 큰 Electron 실행 파일을 배치합니다. 파일
크기는 1.3GB를 초과합니다. 이러한 문제가 발생하는 이유는 Release 타겟 바이너리가
디버그 심볼을 포함하기 때문입니다. 파일 크기를 줄이려면 `create-dist.py` 스크립트를
실행하세요:

```bash
$ ./script/create-dist.py
```

이 스크립트는 매우 작은 배포판을 `dist` 디렉터리에 생성합니다. create-dist.py
스크립트를 실행한 이후부턴 1.3GB에 육박하는 공간을 차지하는 `out/R` 폴더의 바이너리는
삭제해도 됩니다.

또는 `Debug` 타겟만 빌드 할 수 있습니다:

```bash
$ ./script/build.py -c D
```

빌드가 모두 끝나면 `out/D` 디렉터리에서 `electron` 디버그 바이너리를 찾을 수 있습니다.

## 정리하기

빌드 파일들을 정리합니다:

```bash
$ ./script/clean.py
```

## 문제 해결

## libtinfo.so.5 동적 링크 라이브러리를 로드하는 도중 에러가 발생할 경우

미리 빌드된 `clang`은 `libtinfo.so.5`로 링크를 시도합니다. 따라서 플랫폼에 따라
적당한 `libncurses` symlink를 추가하세요:

```bash
$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5
```

## 테스트

프로젝트 코딩 스타일을 확인하려면:

```bash
$ npm run lint
```

테스트를 실행하려면:

```bash
$ ./script/test.py
```

## 고급 주제

기본적인 빌드 구성은 가장 주력인 Linux 배포판에 초점이 맞춰져있으며, 특정 배포판이나
기기에 빌드할 계획이라면 다음 정보들이 도움이 될 것입니다.

### 로컬에서 `libchromiumcontent` 빌드하기

미리 빌드된 `libchromiumcontent`를 사용하는 것을 피하기 위해, `bootstrap.py`
스크립트에 `--build_libchromiumcontent` 스위치를 추가할 수 있습니다:

```bash
$ ./script/bootstrap.py -v --build_libchromiumcontent
```

참고로 `shared_library` 구성은 기본적으로 빌드되어있지 않으며, 다음 모드를 사용하면
`Release` 버전의 Electron만 빌드할 수 있습니다:

```bash
$ ./script/build.py -c R
```

### 다운로드된 `clang` 바이너리 대신 시스템의 `clang` 사용하기

기본적으로 Electron은 Chromium 프로젝트에서 제공하는 미리 빌드된 `clang` 바이너리를
통해 빌드됩니다. 만약 어떤 이유로 시스템에 설치된 `clang`을 사용하여 빌드하고 싶다면,
`bootstrap.py`를 `--clang_dir=<path>` 스위치와 함께 실행함으로써 해결할 수 있습니다.
빌드 스크립트를 이 스위치와 함께 실행할 때 스크립트는 `<path>/bin/`와 같은 경로로
`clang` 바이너리를 찾습니다.

예를 들어 `clang`을 `/user/local/bin/clang`에 설치했다면 다음과 같습니다:

```bash
$ ./script/bootstrap.py -v --build_libchromiumcontent --clang_dir /usr/local
$ ./script/build.py -c R
```

### `clang` 대신 다른 컴파일러 사용하기

Electron을 `g++`과 같은 다른 컴파일러로 빌드하려면, 먼저 `--disable_clang` 스위치를
통해 `clang`을 비활성화 시켜야 하고, 필요하다면 `CC`와 `CXX` 환경 변수도 설정합니다.

예를 들어 GCC 툴체인을 사용하여 빌드한다면 다음과 같습니다:

```bash
$ env CC=gcc CXX=g++ ./script/bootstrap.py -v --build_libchromiumcontent --disable_clang
$ ./script/build.py -c R
```

### 환경 변수

또한 `CC`와 `CXX`와는 별개로, 빌드 구성을 변경하기 위해 다음 환경 변수들을 사용할 수
있습니다:

* `CPPFLAGS`
* `CPPFLAGS_host`
* `CFLAGS`
* `CFLAGS_host`
* `CXXFLAGS`
* `CXXFLAGS_host`
* `AR`
* `AR_host`
* `CC`
* `CC_host`
* `CXX`
* `CXX_host`
* `LDFLAGS`

이 환경 변수는 `bootstrap.py` 스크립트를 실행할 때 설정되어야 하며, `build.py`
스크립트에선 작동하지 않습니다.

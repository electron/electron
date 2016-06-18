# Headless CI 시스템에서 테스팅하기 (Travis, Jenkins)

Chromium을 기반으로 한 Electron은 작업을 위해 디스플레이 드라이버가 필요합니다.
만약 Chromium이 디스플레이 드라이버를 찾기 못한다면, Electron은 그대로 실행에
실패할 것입니다. 따라서 실행하는 방법에 관계없이 모든 테스트를 실행하지 못하게 됩니다.
Electron 기반 애플리케이션을 Travis, Circle, Jenkins 또는 유사한 시스템에서 테스팅을
진행하려면 약간의 설정이 필요합니다. 요점만 말하자면, 가상 디스플레이 드라이버가
필요합니다.

## 가상 디스플레이 드라이버 설정

먼저, [Xvfb](https://en.wikipedia.org/wiki/Xvfb)를 설치합니다. 이것은 X11
디스플레이 서버 프로토콜의 구현이며 모든 그래픽 작업을 스크린 출력없이 인-메모리에서
수행하는 가상 프레임버퍼입니다. 정확히 우리가 필요로 하는 것입니다.

그리고, 가상 xvfb 스크린을 생성하고 DISPLAY라고 불리우는 환경 변수를 지정합니다.
Electron의 Chromium은 자동적으로 `$DISPLAY` 변수를 찾습니다. 따라서 앱의 추가적인
다른 설정이 필요하지 않습니다. 이러한 작업은 Paul Betts의
[xvfb-maybe](https://github.com/paulcbetts/xvfb-maybe)를 통해 자동화 할 수
있습니다: `xvfb-maybe`를 테스트 커맨드 앞에 추가하고 현재 시스템에서 요구하면
이 작은 툴이 자동적으로 xvfb를 설정합니다. Windows와 macOS에선 간단히 아무 작업도
하지 않습니다.

```
## Windows와 macOS에선, 그저 electron-mocha를 호출합니다
## Linux에선, 현재 headless 환경에 있는 경우
## xvfb-run electron-mocha ./test/*.js와 같습니다
xvfb-maybe electron-mocha ./test/*.js
```

### Travis CI

Travis에선, `.travis.yml`이 대충 다음과 같이 되어야 합니다:

```
addons:
  apt:
    packages:
      - xvfb

install:
  - export DISPLAY=':99.0'
  - Xvfb :99 -screen 0 1024x768x24 > /dev/null 2>&1 &
```

### Jenkins

Jenkins는 [Xvfb 플러그인이 존재합니다](https://wiki.jenkins-ci.org/display/JENKINS/Xvfb+Plugin).

### Circle CI

Circle CI는 멋지게도 이미 xvfb와 `$DISPLY` 변수가 준비되어 있습니다. 따라서
[추가적인 설정이 필요하지](https://circleci.com/docs/environment#browsers) 않습니다.

### AppVeyor

AppVeyor는 Windows에서 작동하기 때문에 Selenium, Chromium, Electron과 그 비슷한
툴들을 복잡한 과정 없이 모두 지원합니다. - 설정이 필요하지 않습니다.

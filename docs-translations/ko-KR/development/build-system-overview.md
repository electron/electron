# 빌드 시스템 개요

Electron은 프로젝트 생성을 위해 [gyp](https://gyp.gsrc.io/)를 사용하며
[ninja](https://ninja-build.org/)를 이용하여 빌드합니다. 프로젝트 설정은 `.gyp` 와
`.gypi` 파일에서 볼 수 있습니다.

## gyp 파일

Electron을 빌드 할 때 `gyp` 파일들은 다음과 같은 규칙을 따릅니다:

* `electron.gyp`는 Electron의 빌드 과정 자체를 정의합니다.
* `common.gypi`는 Node가 Chromium과 함께 빌드될 수 있도록 조정한 빌드 설정입니다.
* `vendor/brightray/brightray.gyp`는 `brightray`의 빌드 과정을 정의하고 Chromium
  링킹에 대한 기본적인 설정을 포함합니다.
* `vendor/brightray/brightray.gypi`는 빌드에 대한 일반적인 설정이 포함되어 있습니다.

## 구성요소 빌드

Chromium은 꽤나 큰 프로젝트입니다. 이러한 이유로 인해 최종 링킹 작업은 상당한 시간이
소요될 수 있습니다. 보통 이런 문제는 개발을 어렵게 만듭니다. 우리는 이 문제를 해결하기
위해 Chromium의 "component build" 방식을 도입했습니다. 이는 각각의 컴포넌트를 각각
따로 분리하여 공유 라이브러리로 빌드 합니다. 하지만 이 빌드 방식을 사용하면 링킹 작업은
매우 빨라지지만 실행 파일 크기가 커지고 성능이 저하됩니다.

Electron도 이러한 방식에 상당히 비슷한 접근을 했습니다:
`Debug` 빌드 시 바이너리는 공유 라이브러리 버전의 Chromium 컴포넌트를 사용함으로써
링크 속도를 높이고, `Release` 빌드 시 정적 라이브러리 버전의 컴포넌트를 사용합니다.
이렇게 각 빌드의 단점을 상호 보완하여 디버그 시 빌드 속도는 향상되고 배포판 빌드의
공유 라이브러리의 단점은 개선했습니다.

## 부트스트랩 최소화

Prebuilt된 모든 Chromium 바이너리들은 부트스트랩 스크립트가 실행될 때 다운로드됩니다.
기본적으로 공유 라이브러리와 정적 라이브러리 모두 다운로드되며 최종 전체 파일 크기는
플랫폼에 따라 800MB에서 2GB까지 차지합니다.

기본적으로 (`libchromiumcontent`)는 Amazon Web Service를 통해 다운로드 됩니다. 만약
`LIBCHROMIUMCONTENT_MIRROR` 환경 변수가 설정되어 있으면 부트스트랩은 해당 링크를
사용하여 바이너리를 다운로드 합니다. [libchromiumcontent-qiniu-mirror](https://github.com/hokein/libchromiumcontent-qiniu-mirror)는
libchromiumcontent의 미러입니다. 만약 AWS에 접근할 수 없다면
`export LIBCHROMIUMCONTENT_MIRROR=http://7xk3d2.dl1.z0.glb.clouddn.com/` 미러를
통해 다운로드 할 수 있습니다.

만약 빠르게 Electron의 개발 또는 테스트만 하고 싶다면 `--dev` 플래그를 추가하여 공유
라이브러리만 다운로드할 수 있습니다:

```bash
$ ./script/bootstrap.py --dev
$ ./script/build.py -c D
```

## 두 절차에 따른 프로젝트 생성

Electron은 `Release`와 `Debug` 빌드가 서로 다른 라이브러리 링크 방식을 사용합니다.
하지만 `gyp`는 따로 빌드 설정을 분리하여 라이브러리 링크 방식을 정의하는 방법을
지원하지 않습니다.

이 문제를 해결하기 위해 Electron은 링크 설정을 제어하는 `gyp` 변수
`libchromiumcontent_component`를 사용하고 `gyp`를 실행할 때 단 하나의 타겟만을
생성합니다.

## 타겟 이름

많은 프로젝트에서 타겟 이름을 `Release` 와 `Debug`를 사용하는데 반해 Electron은
`R`과 `D`를 대신 사용합니다. 이유는 가끔 알 수 없는 이유(randomly)로 `Release` 와
`Debug` 중 하나만 빌드 설정에 정의되어 있을때 `gyp`가 크래시를 일으키는데 이유는 앞서
말한 바와 같이 Electron은 한번에 한개의 타겟만을 생성할 수 있기 때문입니다.

이 문제는 개발자에게만 영향을 미칩니다. 만약 단순히 Electron을 rebranding 하기 위해
빌드 하는 것이라면 이 문제에 신경 쓸 필요가 없습니다.

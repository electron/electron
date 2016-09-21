# Windows 스토어 가이드

Windows 8부터, 오래되고 좋은 win32 실행 파일이 새로운 형제를 얻었습니다: Universial
Windows Platform. 새로운 `.appx` 포맷은 Cortana와 푸시 알림과 같은 다수의 강력한
API뿐만 아니라, Windows Store를 통해 설치와 업데이트를 단순화합니다.

Microsoft는 개발자들이 새로운 애플리케이션 모델의 좋은 기능들을 사용할 수 있도록
[Electron 애플리케이션을 `.appx` 패키지로 컴파일시키는 도구를 개발했습니다][electron-windows-store].
이 가이드는 이 도구를 사용하는 방법과 Electron AppX 패키지의 호환성과 한정 사항을
설명합니다.

## 기본 상식과 요구 사항

Windows 10 "기념일 업데이트"는 win32 `.exe` 바이너리를 가상화된 파일 시스템과
레지스트리를 함께 실행시킬 수 있도록 만들었습니다. 두 가지 모두 실행 중인
애플리케이션과 Windows 컨테이너 안의 인스톨러에 의해 컴파일되는 동안 만들어지며,
설치가 되는 동안 Windows가 확실하게 어떤 변경 사항이 운영 체제에 적용되는지 식별할 수
있도록 합니다. 가상 파일 시스템과 가상 레지스트리를 페어링 하는 실행 파일은 Windows가
한 번의 클릭으로 설치와 삭제를 할 수 있도록 만듭니다.

더 나아가서, exe는 appx 모델 안에서 실행됩니다 - 이 말은 즉 Universial Windows
Platform에서 제공되는 수많은 API를 사용할 수 있다는 이야기입니다. 더 많은 기능을
사용하기 위해, Electron 애플리케이션은 백그라운드로 실행된 UWP 앱과 페어링 하여
`exe`와 같이 실행할 수 있습니다 - 이렇게 헬퍼와 비슷하게 실행되고 작업을 실행하기 위해
백그라운드에서 작동하며, 푸시 알림을 받거나, 다른 UWP 애플리케이션과 통신하는 역할을
합니다.

현재 존재하는 Electron 애플리케이션을 컴파일 하려면, 다음 요구 사항을 충족해야 합니다:


* Windows 10 기념일 업데이트 (2016년 8월 2일 발매)
* Windows 10 SDK, [여기서 다운로드][windows-sdk]

그리고 CLI에서 `electron-windows-store`를 설치합니다:

```
npm install -g electron-windows-store
```

## Step 1: Electron 어플리케이션 패키징

[electron-packager](https://github.com/electron-userland/electron-packager)를
사용하여 애플리케이션을 패키징합니다. (또는 비슷한 도구를 사용합니다) 마지막으로 최종
애플리케이션에선 `node_modules`가 필요 없으며 그저 애플리케이션의 크기를 늘릴 뿐이니
확실하게 지웠는지 확인합니다.

출력된 파일들은 대충 다음과 같아야 합니다:

```
├── Ghost.exe
├── LICENSE
├── content_resources_200_percent.pak
├── content_shell.pak
├── d3dcompiler_47.dll
├── ffmpeg.dll
├── icudtl.dat
├── libEGL.dll
├── libGLESv2.dll
├── locales
│   ├── am.pak
│   ├── ar.pak
│   ├── [...]
├── natives_blob.bin
├── node.dll
├── resources
│   ├── app
│   └── atom.asar
├── snapshot_blob.bin
├── squirrel.exe
├── ui_resources_200_percent.pak
└── xinput1_3.dll
```

## `electron-windows-store` 실행하기

관리자 권한의 PowerShell ("관리자 권한으로 실행")을 실행하고, 디렉토리 입력과 출력,
애플리케이션의 이름과 버전, 마지막으로 `node_modules`를 평탄화시키는 인수들과 함께
`electron-windows-store`를 실행합니다.

```
electron-windows-store `
    --input-directory C:\myelectronapp `
    --output-directory C:\output\myelectronapp `
    --flatten true `
    --package-version 1.0.0.0 `
    --package-name myelectronapp
```

명령이 실행되면, 도구는 다음과 같이 작동합니다: Electron 애플리케이션을 입력으로 받고,
`node_modules`를 평탄화하고 애플리케이션을 `app.zip`으로 압축합니다. 그리고
인스톨러와 Windows Container를 사용하여, "확장된" AppX 패키지를 만듭니다 -
Windows Application Manifest (`AppXManifest.xml`)와 동시에 가상 파일 시스템과 가상
레지스트리를 포함하여 출력 폴더로 내보냅니다.

확장된 AppX 파일이 만들어지면, 도구는 Windows App Packager (`MakeAppx.exe`)를
사용하여 디스크의 파일로부터 단일-파일 AppX 패키지를 생성해냅니다. 최종적으로, 이
도구는 새 AppX 패키지에 서명하기 위해 컴퓨터에서 신뢰된 인증서를 만드는 데 사용할 수
있습니다. 서명된 AppX 패키지로, CLI는 자동으로 기기에 패키지를 설치할 수 있습니다.

## Step 3: AppX 패키지 사용하기

일반 사용자들이 패키지를 실행하기 위해서는 "기념일 업데이트" 된 Windows 10 이
필요합니다. - Windows 업데이트 하는 방법은 [여기][how-to-update]에서 찾을 수
있습니다.

기존 UWP 앱과 반대로, 패키지된 앱은 현재 수동 확인 절차가 필요합니다. 이것은
[여기][centennial-campaigns]에서 적용할 수 있습니다. 반면에, 사용자는
더블클릭으로 패키지를 설치할 수 있습니다. 더 쉬운 설치 방법을 찾고 있다면,
스토어에 제출하지 않아도 됩니다. (일반적으로 기업과 같은) 관리되는 환경에서는,
`Add-AppxPackage` [PowerShell Cmdlet 은 자동화된 방식으로 설치하는데 사용될 수
있습니다][add-appxpackage].

또 다른 중요한 제약은 컴파일된 AppX 패키지는 여전히 win32 실행 파일이 담겨있다는
것입니다 - 따라서 Xbox, HoloLens, Phone에서 실행할 수 없습니다.

## Optional: 백그라운드 작업을 사용하여 UWP 기능 추가

필요하다면 Windows 10의 모든 기능을 사용하기 위해 보이지 않는 UWP 백그라운드 작업과
Electron 앱을 연결할 수 있습니다 - 푸시 알림, 코타나 통합, 라이브 타일등.

Electron 앱이 토스트 알림이나 라이브 타일을 사용할 수 있도록 하는 방법을 알아보려면
[Microsoft가 제공하는 샘플을 참고하세요][background-task].

## Optional: 컨테이너 가상화를 사용하여 변환

AppX 패키지를 만드려면, `electron-windows-store` CLI를 통해 대부분의 Electron 앱을
그대로 변환할 수 있습니다. 하지만, 커스텀 인스톨러를 사용하고 있거나, 패키지를
생성하는데 어떠한 문제라도 겪고있다면, Windows Container를 사용하여 패키지를 생성하는
시도를 해볼 수 있습니다 - 이 모드를 사용하면, CLI가 어플리케이션이 정확하게 운영체제에
대해 수행하는 작업이 어떤 변경 사항을 만드는지를 결정하기 위해 어플리케이션을 빈 Windows
Container에서 설치하고 실행할 것입니다.

처음 CLI를 실행하기 전에, "Windows Desktop App Converter"를 먼저 설정해야 합니다.
이 작업은 약 몇 분 정도 소요됩니다. 하지만 걱정하지 않아도 됩니다 - 이 작업은 딱 한
번만 하면 됩니다. Desktop App Converter는 [여기][app-converter]에서 다운로드 받을
수 있고, `DesktopAppConverter.zip`와 `BaseImage-14316.wim` 두 파일을 모두 받아야
합니다.

1. `DesktopAppConverter.zip`의 압축을 풉니다. 그다음 PowerShell을 관리자 권한으로
  실행하고 압축을 푼 위치로 이동합니다. (실행하는 모든 명령에 "관리자 권한"을
  적용하려면 `Set-ExecutionPolicy bypass`를 실행하면 됩니다)
2. 그리고, `.\DesktopAppConverter.ps1 -Setup -BaseImage .\BaseImage-14316.wim`를
  실행하여 Windows 베이스 이미지 (`BaseImage-14316.wim`)를 Desktop App Converter로
  전달하고 설치를 진행합니다.
3. 만약 위 명령이 재시작을 요구하면, 기기를 재시작하고 위 명령을 다시 실행시키세요.

설치가 완료되면, 컴파일할 Electron 애플리케이션 경로로 이동합니다.

[windows-sdk]: https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk
[app-converter]: https://www.microsoft.com/en-us/download/details.aspx?id=51691
[add-appxpackage]: https://technet.microsoft.com/en-us/library/hh856048.aspx
[electron-packager]: https://github.com/electron-userland/electron-packager
[electron-windows-store]: https://github.com/catalystcode/electron-windows-store
[background-task]: https://github.com/felixrieseberg/electron-uwp-background
[centennial-campaigns]: https://developer.microsoft.com/en-us/windows/projects/campaigns/desktop-bridge
[how-to-update]: https://blogs.windows.com/windowsexperience/2016/08/02/how-to-get-the-windows-10-anniversary-update

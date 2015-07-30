# NativeImage

Electron은 파일 경로나 `NativeImage` 인스턴스를 전달하여 사용하는 이미지 API를 가지고 있습니다. `null`을 전달할 경우 빈 이미지가 사용됩니다.

예를 들어 트레이 메뉴를 만들거나 윈도우의 아이콘을 설정할 때 다음과 같이 `문자열`인 파일 경로를 전달할 수 있습니다:

```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png');
var window = new BrowserWindow({icon: '/Users/somebody/images/window.png'});
```

또는 클립보드로부터 이미지를 읽어올 수 있습니다:

```javascript
var clipboard = require('clipboard');
var image = clipboard.readImage();
var appIcon = new Tray(image);
```

## 지원하는 포맷

현재 `PNG` 와 `JPEG` 포맷을 지원하고 있습니다. 손실 없는 이미지 압축과 투명도 지원을 위해 `PNG` 사용을 권장합니다.

## 고해상도 이미지

플랫폼이 high-DPI를 지원하는 경우 `@2x`와 같이 이미지의 파일명 뒤에 접미사를 추가하여 고해상도 이미지로 지정할 수 있습니다.

예를 들어 `icon.png` 라는 기본 해상도의 이미지를 기준으로 크기를 두 배로 늘린 이미지를 `icon@2x.png`와 같이 이름을 지정하면 고해상도 이미지로 처리됩니다.

서로 다른 해상도(DPI)의 이미지를 지원하고 싶다면 다중 해상도의 이미지를 접미사를 붙여 한 폴더에 넣으면 됩니다. 이 이미지를 사용(로드)할 땐 접미사를 붙이지 않습니다:

```text
images/
├── icon.png
├── icon@2x.png
└── icon@3x.png
```


```javascript
var appIcon = new Tray('/Users/somebody/images/icon.png');
```

지원하는 DPI 접미사는 다음과 같습니다:

* `@1x`
* `@1.25x`
* `@1.33x`
* `@1.4x`
* `@1.5x`
* `@1.8x`
* `@2x`
* `@2.5x`
* `@3x`
* `@4x`
* `@5x`

## 템플릿 이미지

템플릿 이미지는 검은색과 명확한 색상(알파 채널)으로 이루어져 있습니다.
템플릿 이미지는 단독 이미지로 사용되지 않고 다른 컨텐츠와 혼합되어 최종 외관 만드는데 사용됩니다.

가장 일반적으로 템플릿 이미지는 밝고 어두운 테마 색상으로 변경할 수 있는 메뉴 바 아이콘 등에 사용되고 있습니다.

템플릿 이미지는 Mac 운영체제만 지원합니다.

템플릿 이미지를 지정하려면 다음 예제와 같이 파일명에 `Template` 문자열을 추가해야 합니다:

* `xxxTemplate.png`
* `xxxTemplate@2x.png`

## nativeImage.createEmpty()

빈 `NativeImage` 인스턴스를 만듭니다.

## nativeImage.createFromPath(path)

* `path` String

`path`로부터 이미지를 로드하여 새로운 `NativeImage` 인스턴스를 만듭니다.

## nativeImage.createFromBuffer(buffer[, scaleFactor])

* `buffer` [Buffer][buffer]
* `scaleFactor` Double

`buffer`로부터 이미지를 로드하여 새로운 `NativeImage` 인스턴스를 만듭니다. `scaleFactor`는 1.0이 기본입니다.

## nativeImage.createFromDataUrl(dataUrl)

* `dataUrl` String

`dataUrl`로부터 이미지를 로드하여 새로운 `NativeImage` 인스턴스를 만듭니다.

## Class: NativeImage

이미지를 표현한 클래스입니다.

### NativeImage.toPng()

`PNG` 이미지를 인코딩한 데이터를 [Buffer][buffer]로 반환합니다.

### NativeImage.toJpeg(quality)

* `quality` Integer (0 - 100 사이의 값)

`JPEG` 이미지를 인코딩한 데이터를 [Buffer][buffer]로 반환합니다.

### NativeImage.toDataUrl()

이미지의 data URL을 반환합니다.

### NativeImage.isEmpty()

이미지가 비었는지를 체크합니다.

### NativeImage.getSize()

이미지의 사이즈를 반환합니다.

### NativeImage.setTemplateImage(option)

* `option` Boolean

해당 이미지를 템플릿 이미지로 설정합니다.

### NativeImage.isTemplateImage()

이미지가 템플릿 이미지인지 확인합니다.

[buffer]: https://iojs.org/api/buffer.html#buffer_class_buffer

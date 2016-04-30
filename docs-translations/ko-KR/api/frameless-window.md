# Frameless Window

Frameless Window는 [창 테두리](https://developer.mozilla.org/ko/docs/Glossary/Chrome)가
없는 윈도우를 말합니다. 이 기능은 윈도우의 일부분인 툴바와 같이 웹 페이지의 일부분이
아닌 부분을 보이지 않도록 합니다. [`BrowserWindow`](browser-window.md) 클래스의
옵션에서 설정할 수 있습니다.

## Frameless Window 만들기

Frameless Window를 만드려면 [BrowserWindow](browser-window.md) 객체의
`options` 객체에서 `frame` 옵션을 `false`로 지정하면 됩니다:

```javascript
const BrowserWindow = require('electron').BrowserWindow;
var win = new BrowserWindow({ width: 800, height: 600, frame: false });
```

### 최신 OS X에서 사용할 수 있는 대안

OS X 10.10 Yosemite 이후의 최신 버전부터는 테두리가 없는 창을 만들 때 새로운 방법을
사용할 수 있습니다. `frame` 옵션을 `false`로 지정하여 제목과 창 구성 요소를 모두
비활성화하는 대신 새로운 `titleBarStyle` 옵션을 통해 제목만 숨기고 창 구성 요소
("흔히 신호등으로 알고 있는")의 기능과 창 크기를 그대로 유지할 수 있습니다:

```javascript
var win = new BrowserWindow({ 'titleBarStyle': 'hidden' });
```

## 투명한 창 만들기

Frameless Window 창의 배경을 투명하게 만들고 싶다면 `transparent` 옵션을 `true`로
바꿔주기만 하면됩니다:

```javascript
var win = new BrowserWindow({ transparent: true, frame: false });
```

### API의 한계

* 투명한 영역을 통과하여 클릭할 수 없습니다. 우리는 이 문제를 해결하기 위해 API를
  제공할 예정이며 자세한 내용은
  [이슈](https://github.com/electron/electron/issues/1335)를 참고하세요.
* 투명한 창은 크기를 조절할 수 없습니다. `resizable` 속성을 `true`로 할 경우 몇몇
  플랫폼에선 크래시가 일어납니다.
* `blur` 필터는 웹 페이지에서만 적용됩니다. 윈도우 아래 콘텐츠에는 블러 효과를 적용할
  방법이 없습니다. (예시: 유저의 시스템에 열린 다른 어플리케이션)
* Windows에선 DWM(데스크톱 창 관리자)가 비활성화되어 있을 경우 투명한 창이 작동하지
  않습니다.
* Linux를 사용할 경우 [alpha channel doesn't work on some NVidia drivers](https://code.google.com/p/chromium/issues/detail?id=369209)
  upstream 버그가 있는 관계로 투명한 창 기능을 사용하려면 CLI 옵션에
  `--enable-transparent-visuals --disable-gpu`을 추가해야 합니다. 이 옵션은 GPU의
  사용을 중단하고 윈도우를 생성하는데 ARGB를 사용할 수 있도록 해줍니다.
* OS X(Mac)에선 네이티브 창에서 보여지는 그림자가 투명한 창에선 보이지 않습니다.

## 드래그 가능 위치 지정

기본적으로 Frameless Window는 드래그 할 수 없습니다. 어플리케이션의 CSS에서 특정
범위를 `-webkit-app-region: drag`로 지정하면 OS의 기본 타이틀 바 처럼 드래그 되도록
할 수 있습니다. 그리고 `-webkit-app-region: no-drag`를 지정해서 드래그 불가능 영역을
만들 수도 있습니다. 현재 사각형 형태의 범위만 지원합니다.

창 전체를 드래그 가능하게 만드려면 `-webkit-app-region: drag`을 `body`의 스타일에
지정하면 됩니다:

```html
<body style="-webkit-app-region: drag">
</body>
```

참고로 창 전체를 드래그 영역으로 지정할 경우 사용자가 버튼을 클릭할 수 없게 되므로
버튼은 드래그 불가능 영역으로 지정해야 합니다:

```css
button {
  -webkit-app-region: no-drag;
}
```

따로 커스텀 타이틀 바를 만들어 사용할 때는 타이틀 바 내부의 모든 버튼을 드래그 불가
영역으로 지정해야 합니다.

## 텍스트 선택

Frameless Window에서 텍스트가 선택되는 드래그 동작은 혼란을 야기할 수 있습니다. 예를
들어 타이틀 바를 드래그 할 때 타이틀 바의 텍스트를 실수로 선택할 수 있습니다. 이를
방지하기 위해 다음과 같이 드래그 영역의 텍스트 선택 기능을 비활성화해야 할 필요가
있습니다:

```css
.titlebar {
  -webkit-user-select: none;
  -webkit-app-region: drag;
}
```

## 컨텍스트 메뉴

몇몇 플랫폼에선 드래그 가능 영역이 non-client 프레임으로 처리됩니다. 이러한 플랫폼에선
드래그 가능 영역에서 오른쪽 클릭 할 경우 시스템 메뉴가 팝업 됩니다. 이러한 이유로
컨텍스트 메뉴 지정 시 모든 플랫폼에서 정상적으로 작동하게 하려면 커스텀 컨텍스트 메뉴를
드래그 영역 내에 만들어선 안됩니다.

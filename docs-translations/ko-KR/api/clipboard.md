# clipboard

> 시스템 클립보드에 복사와 붙여넣기를 수행합니다.

다음 예시는 클립보드에 문자열을 쓰는 방법을 보여줍니다:

```javascript
const {clipboard} = require('electron')
clipboard.writeText('Example String')
```

X Window 시스템에선 selection 클립보드도 존재합니다. 이를 사용하려면 인수 뒤에
`selection` 문자열을 같이 지정해주어야 합니다:

```javascript
const {clipboard} = require('electron')
clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
```

## Methods

`clipboard` 모듈은 다음과 같은 메서드를 가지고 있습니다:

**참고:** Experimental 마크가 붙은 API는 실험적인 기능이며 차후 최신 버전에서 제거될
수 있습니다.

### `clipboard.readText([type])`

* `type` String (optional)

Returns `String` - 일반 텍스트 형식의 클립보드의 내용.

### `clipboard.writeText(text[, type])`

* `text` String
* `type` String (optional)

클립보드에 `plain text`로 문자열을 씁니다.

### `clipboard.readHTML([type])`

* `type` String (optional)

returns `String` - 마크업 형식의 클립보드의 내용.

### `clipboard.writeHTML(markup[, type])`

* `markup` String
* `type` String (optional)

클립보드에 `markup`으로 씁니다.

### `clipboard.readImage([type])`

* `type` String (optional)

Returns `NativeImage` - [NativeImage](native-image.md) 형식의 클립보드의 내용.

### `clipboard.writeImage(image[, type])`

* `image` [NativeImage](native-image.md)
* `type` String (optional)

클립보드에 `image`를 씁니다.

### `clipboard.readRTF([type])`

* `type` String (optional)

Returns `String` - RTF 형식의 클립보드 내용.

### `clipboard.writeRTF(text[, type])`

* `text` String
* `type` String (optional)

클립보드에 `text`를 RTF 형식으로 씁니다.

### `clipboard.readBookmark()` _macOS_ _Windows_

Returns `Object`:

* `title` String
* `url` String

클립보드로부터 북마크 형식으로 표현된 `title`와 `url` 키를 담은 객체를 반환합니다.
`title`과 `url` 값들은 북마크를 사용할 수 없을 때 빈 문자열을 포함합니다.

### `clipboard.writeBookmark(title, url[, type])` _macOS_ _Windows_

* `title` String
* `url` String
* `type` String (optional)

`title`과 `url`을 클립보드에 북마크 형식으로 씁니다.

**참고:** 윈도우의 대부분의 앱은 북마크 붙여넣기를 지원하지 않습니다.
`clipboard.write` 를 통해 북마크와 대체 텍스트를 클립보드에 쓸 수 있습니다.

```javascript
clipboard.write({
  text: 'http://electron.atom.io',
  bookmark: 'Electron Homepage'
})
```

### `clipboard.clear([type])`

* `type` String (optional)

클립보드에 저장된 모든 콘텐츠를 삭제합니다.

### `clipboard.availableFormats([type])`

Return `String[]` - 클립보드 `type` 에 지원되는 형식의 배열.

### `clipboard.has(data[, type])`

* `data` String
* `type` String (optional)

Returns `Boolean` - 클립보드가 지정한 `data` 의 형식을 지원하는지 여부.

```javascript
const {clipboard} = require('electron')
console.log(clipboard.has('<p>selection</p>'))
```

### `clipboard.read(data[, type])` _Experimental_

* `data` String
* `type` String (optional)

Returns `String` - 클립보드로부터 `data`를 읽습니다.

### `clipboard.write(data[, type])` _Experimental_

* `data` Object
  * `text` String
  * `html` String
  * `image` [NativeImage](native-image.md)
  * `rtf` String
  * `bookmark` String - `text`에 있는 URL의 텍스트.
* `type` String (optional)

```javascript
const {clipboard} = require('electron')
clipboard.write({text: 'test', html: '<b>test</b>'})
```

`data`를 클립보드에 씁니다.

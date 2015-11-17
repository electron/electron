# BrowserWindow

`BrowserWindow` 클래스는 브라우저 창(윈도우 창)을 만드는 역할을 담당합니다.

다음 예제는 윈도우 창을 생성합니다:

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600, show: false });
win.on('closed', function() {
  win = null;
});

win.loadURL('https://github.com');
win.show();
```

또한 [Frameless Window](frameless-window.md) API를 사용하여 창 테두리가 없는 윈도우 창을 생성할 수 있습니다.

## Class: BrowserWindow

`BrowserWindow`는 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)를 상속받은 클래스 입니다.

`BrowserWindow`는 `options`를 통해 네이티브 속성을 포함한 새로운 윈도우 창을 생성합니다.

### `new BrowserWindow(options)`

`options` 객체에서 사용할 수 있는 속성들:

* `width` Integer - 윈도우 창의 가로 너비.
* `height` Integer - 윈도우 창의 세로 높이.
* `x` Integer - 화면을 기준으로 창 좌측을 오프셋 한 위치.
* `y` Integer - 화면을 기준으로 창 상단을 오프셋 한 위치.
* `useContentSize` Boolean - `width`와 `height`를 웹 페이지의 크기로 사용합니다.
  이 속성을 사용하면 웹 페이지의 크기에 윈도우 프레임 크기가 추가되므로 실제 창은 조금 더 커질 수 있습니다.
* `center` Boolean - 윈도우 창을 화면 정 중앙에 위치시킵니다.
* `minWidth` Integer - 윈도우 창의 최소 가로 너비.
* `minHeight` Integer - 윈도우 창의 최소 세로 높이.
* `maxWidth` Integer - 윈도우 창의 최대 가로 너비.
* `maxHeight` Integer - 윈도우 창의 최대 세로 높이.
* `resizable` Boolean - 윈도우 창의 크기를 재조정 할 수 있는지 여부.
* `alwaysOnTop` Boolean - 윈도우 창이 언제나 다른 창들 위에 유지되는지 여부.
* `fullscreen` Boolean - 윈도우 창의 전체화면 활성화 여부.
  `false`로 지정했을 경우 OS X에선 전체화면 버튼이 숨겨지거나 비활성화됩니다.
* `skipTaskbar` Boolean - 작업 표시줄 어플리케이션 아이콘 표시 여부.
* `kiosk` Boolean - Kiosk(키오스크) 모드.
* `title` String - 기본 윈도우 창 제목.
* `icon` [NativeImage](native-image.md) - 윈도우 아이콘, 생략하면 실행 파일의 아이콘이 대신 사용됩니다.
* `show` Boolean - 윈도우가 생성되면 보여줄지 여부.
* `frame` Boolean - `false`로 지정하면 창을 [Frameless Window](frameless-window.md) 형태로 생성합니다.
* `acceptFirstMouse` Boolean - 윈도우가 비활성화 상태일 때 내부 컨텐츠 클릭 시 활성화 되는 동시에 단일 mouse-down 이벤트를 발생시킬지 여부.
* `disableAutoHideCursor` Boolean - 파이핑중 자동으로 커서를 숨길지 여부.
* `autoHideMenuBar` Boolean - `Alt`를 누르지 않는 한 어플리케이션 메뉴바를 숨길지 여부.
* `enableLargerThanScreen` Boolean - 윈도우 창 크기가 화면 크기보다 크게 재조정 될 수 있는지 여부.
* `backgroundColor` String - 16진수로 표현된 윈도우의 배경 색. `#66CD00` 또는 `#FFF`가 사용될 수 있습니다.
  이 속성은 Linux와 Windows에만 구현되어 있습니다.
* `darkTheme` Boolean - 설정에 상관 없이 무조건 어두운 윈도우 테마를 사용합니다. 몇몇 GTK+3 데스크톱 환경에서만 작동합니다.
* `transparent` Boolean - 윈도우 창을 [투명화](frameless-window.md)합니다.
* `type` String - 윈도우 창 종류를 지정합니다.
  사용할 수 있는 창 종류는 `desktop`, `dock`, `toolbar`, `splash`, `notification`와 같습니다.
  이 속성은 Linux에서만 작동합니다.
* `standardWindow` Boolean - OS X의 표준 윈도우를 텍스쳐 윈도우 대신 사용합니다. 기본 값은 `true`입니다.
* `titleBarStyle` String, OS X - 윈도우 타이틀 바 스타일을 지정합니다. 이 속성은 OS X 10.10 Yosemite 이후 버전만 지원합니다.
  다음 3가지 종류의 값을 사용할 수 있습니다:
  * `default` 또는 미지정: 표준 Mac 회색 불투명 스타일을 사용합니다.
  * `hidden`: 타이틀 바를 숨기고 컨텐츠 전체를 윈도우 크기에 맞춥니다.
    타이틀 바는 없어지지만 표준 창 컨트롤 ("신호등 버튼")은 왼쪽 상단에 유지됩니다.
  * `hidden-inset`: `hidden` 타이틀 바 속성과 함께 신호등 버튼이 윈도우 모서리로부터 약간 더 안쪽으로 들어가도록합니다.
* `webPreferences` Object - 웹 페이지 기능을 설정합니다. 사용할 수 있는 속성은 다음과 같습니다:
  * `nodeIntegration` Boolean - node(node.js) 통합 여부. 기본값은 `true`입니다.
  * `preload` String - 스크립트를 지정하면 페이지 내의 다른 스크립트가 작동하기 전에 로드됩니다.
    여기서 지정한 스크립트는 페이지의 node 통합 활성화 여부에 상관없이 언제나 모든 node API에 접근할 수 있습니다.
    그리고 `preload` 스크립트의 경로는 절대 경로로 지정해야 합니다.
  * `partition` String - 페이지에서 사용할 세션을 지정합니다. 만약 `partition`이
    `persist:`로 시작하면 페이지는 지속성 세션을 사용하며 다른 모든 앱 내의 페이지에서 같은 `partition`을 사용할 수 있습니다.
    만약 `persist:` 접두어로 시작하지 않으면 페이지는 인-메모리 세션을 사용합니다. 여러 페이지에서 같은 `partition`을 지정하면
    같은 세션을 공유할 수 있습니다. `partition`을 지정하지 않으면 어플리케이션의 기본 세션이 사용됩니다.
  * `zoomFactor` Number - 페이지의 기본 줌 값을 지정합니다. 예를 들어 `300%`를 표현하려면 `3.0`으로 지정합니다.
  * `javascript` Boolean
  * `webSecurity` Boolean - `false`로 지정하면 same-origin 정책을 비활성화합니다. (이 속성은 보통 사람에 의해 웹 사이트를 테스트할 때 사용합니다)
    그리고 `allowDisplayingInsecureContent`와 `allowRunningInsecureContent`이 사용자로부터 `true`로 지정되지 않은 경우 `true`로 지정합니다.
  * `allowDisplayingInsecureContent` Boolean - https 페이지에서 http URL에서 로드한 이미지 같은 리소스를 표시할 수 있도록 허용합니다.
  * `allowRunningInsecureContent` Boolean - https 페이지에서 http URL에서 로드한 JavaScript와 CSS 또는 플러그인을 실행시킬 수 있도록 허용합니다.
  * `images` Boolean
  * `java` Boolean
  * `textAreasAreResizable` Boolean
  * `webgl` Boolean
  * `webaudio` Boolean
  * `plugins` Boolean - 어떤 플러그인이 활성화되어야 하는지 지정합니다.
  * `experimentalFeatures` Boolean
  * `experimentalCanvasFeatures` Boolean
  * `overlayScrollbars` Boolean
  * `overlayFullscreenVideo` Boolean
  * `sharedWorker` Boolean
  * `directWrite` Boolean - Windows에서 폰트 랜더링을 위해 DirectWrite를 사용하는지를 지정합니다.
  * `pageVisibility` Boolean - 현재 윈도우의 가시성을 반영하는 대신 페이지가 visible 또는 hidden 중 지정된 상태를 계속 유지하도록 합니다.
    이 속성을 `true`로 지정하면 DOM 타이머의 스로틀링을 방지할 수 있습니다.

## Events

`BrowserWindow` 객체는 다음과 같은 이벤트를 발생시킵니다:

**참고:** 몇몇 이벤트는 라벨에 특정한 OS에서만 작동합니다.

### Event: 'page-title-updated'

Returns:

* `event` Event

문서의 제목이 변경될 때 발생하는 이벤트 입니다.
`event.preventDefault()`를 호출하여 네이티브 윈도우의 제목이 변경되는 것을 방지할 수 있습니다.

### Event: 'close'

Returns:

* `event` Event

Emitted when the window is going to be closed. It's emitted before the
`beforeunload` and `unload` event of the DOM. Calling `event.preventDefault()`
will cancel the close.

Usually you would want to use the `beforeunload` handler to decide whether the
window should be closed, which will also be called when the window is
reloaded. In Electron, returning an empty string or `false` would cancel the
close. For example:

```javascript
window.onbeforeunload = function(e) {
  console.log('I do not want to be closed');

  // Unlike usual browsers, in which a string should be returned and the user is
  // prompted to confirm the page unload, Electron gives developers more options.
  // Returning empty string or false would prevent the unloading now.
  // You can also use the dialog API to let the user confirm closing the application.
  e.returnValue = false;
};
```

### Event: 'closed'

Emitted when the window is closed. After you have received this event you should
remove the reference to the window and avoid using it anymore.

### Event: 'unresponsive'

Emitted when the web page becomes unresponsive.

### Event: 'responsive'

Emitted when the unresponsive web page becomes responsive again.

### Event: 'blur'

Emitted when the window loses focus.

### Event: 'focus'

Emitted when the window gains focus.

### Event: 'maximize'

Emitted when window is maximized.

### Event: 'unmaximize'

Emitted when the window exits from maximized state.

### Event: 'minimize'

Emitted when the window is minimized.

### Event: 'restore'

Emitted when the window is restored from minimized state.

### Event: 'resize'

Emitted when the window is getting resized.

### Event: 'move'

Emitted when the window is getting moved to a new position.

__Note__: On OS X this event is just an alias of `moved`.

### Event: 'moved' _OS X_

Emitted once when the window is moved to a new position.

### Event: 'enter-full-screen'

Emitted when the window enters full screen state.

### Event: 'leave-full-screen'

Emitted when the window leaves full screen state.

### Event: 'enter-html-full-screen'

Emitted when the window enters full screen state triggered by html api.

### Event: 'leave-html-full-screen'

Emitted when the window leaves full screen state triggered by html api.

### Event: 'app-command':

Emitted when an [App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)
is invoked. These are typically related to keyboard media keys or browser
commands, as well as the "Back" button built into some mice on Windows.

```js
someWindow.on('app-command', function(e, cmd) {
  // Navigate the window back when the user hits their mouse back button
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack();
  }
});
```

## Methods

`BrowserWindow` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `BrowserWindow.getAllWindows()`

Returns an array of all opened browser windows.

### `BrowserWindow.getFocusedWindow()`

Returns the window that is focused in this application.

### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](web-contents.md)

Find a window according to the `webContents` it owns.

### `BrowserWindow.fromId(id)`

* `id` Integer

Find a window according to its ID.

### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

Adds DevTools extension located at `path`, and returns extension's name.

The extension will be remembered so you only need to call this API once, this
API is not for programming use.

### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

Remove the DevTools extension whose name is `name`.

## Instance Properties

Objects created with `new BrowserWindow` have the following properties:

```javascript
// In this example `win` is our instance
var win = new BrowserWindow({ width: 800, height: 600 });
```

### `win.webContents`

The `WebContents` object this window owns, all web page related events and
operations will be done via it.

See the [`webContents` documentation](web-contents.md) for its methods and
events.

### `win.id`

The unique ID of this window.

## Instance Methods

Objects created with `new BrowserWindow` have the following instance methods:

**Note:** Some methods are only available on specific operating systems and are labeled as such.

### `win.destroy()`

Force closing the window, the `unload` and `beforeunload` event won't be emitted
for the web page, and `close` event will also not be emitted
for this window, but it guarantees the `closed` event will be emitted.

You should only use this method when the renderer process (web page) has
crashed.

### `win.close()`

Try to close the window, this has the same effect with user manually clicking
the close button of the window. The web page may cancel the close though, see
the [close event](#event-close).

### `win.focus()`

Focus on the window.

### `win.isFocused()`

Returns a boolean, whether the window is focused.

### `win.show()`

Shows and gives focus to the window.

### `win.showInactive()`

Shows the window but doesn't focus on it.

### `win.hide()`

Hides the window.

### `win.isVisible()`

Returns a boolean, whether the window is visible to the user.

### `win.maximize()`

Maximizes the window.

### `win.unmaximize()`

Unmaximizes the window.

### `win.isMaximized()`

Returns a boolean, whether the window is maximized.

### `win.minimize()`

Minimizes the window. On some platforms the minimized window will be shown in
the Dock.

### `win.restore()`

Restores the window from minimized state to its previous state.

### `win.isMinimized()`

Returns a boolean, whether the window is minimized.

### `win.setFullScreen(flag)`

* `flag` Boolean

Sets whether the window should be in fullscreen mode.

### `win.isFullScreen()`

Returns a boolean, whether the window is in fullscreen mode.

### `win.setAspectRatio(aspectRatio[, extraSize])` _OS X_

* `aspectRatio` The aspect ratio we want to maintain for some portion of the
content view.
* `extraSize` Object (optional) - The extra size not to be included while
maintaining the aspect ratio. Properties:
  * `width` Integer
  * `height` Integer

This will have a window maintain an aspect ratio. The extra size allows a
developer to have space, specified in pixels, not included within the aspect
ratio calculations. This API already takes into account the difference between a
window's size and its content size.

Consider a normal window with an HD video player and associated controls.
Perhaps there are 15 pixels of controls on the left edge, 25 pixels of controls
on the right edge and 50 pixels of controls below the player. In order to
maintain a 16:9 aspect ratio (standard aspect ratio for HD @1920x1080) within
the player itself we would call this function with arguments of 16/9 and
[ 40, 50 ]. The second argument doesn't care where the extra width and height
are within the content view--only that they exist. Just sum any extra width and
height areas you have within the overall content view.

### `win.setBounds(options)`

`options` Object, properties:

* `x` Integer
* `y` Integer
* `width` Integer
* `height` Integer

Resizes and moves the window to `width`, `height`, `x`, `y`.

### `win.getBounds()`

Returns an object that contains window's width, height, x and y values.

### `win.setSize(width, height)`

* `width` Integer
* `height` Integer

Resizes the window to `width` and `height`.

### `win.getSize()`

Returns an array that contains window's width and height.

### `win.setContentSize(width, height)`

* `width` Integer
* `height` Integer

Resizes the window's client area (e.g. the web page) to `width` and `height`.

### `win.getContentSize()`

Returns an array that contains window's client area's width and height.

### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

Sets the minimum size of window to `width` and `height`.

### `win.getMinimumSize()`

Returns an array that contains window's minimum width and height.

### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

Sets the maximum size of window to `width` and `height`.

### `win.getMaximumSize()`

Returns an array that contains window's maximum width and height.

### `win.setResizable(resizable)`

* `resizable` Boolean

Sets whether the window can be manually resized by user.

### `win.isResizable()`

Returns whether the window can be manually resized by user.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

Sets whether the window should show always on top of other windows. After
setting this, the window is still a normal window, not a toolbox window which
can not be focused on.

### `win.isAlwaysOnTop()`

Returns whether the window is always on top of other windows.

### `win.center()`

Moves window to the center of the screen.

### `win.setPosition(x, y)`

* `x` Integer
* `y` Integer

Moves window to `x` and `y`.

### `win.getPosition()`

Returns an array that contains window's current position.

### `win.setTitle(title)`

* `title` String

Changes the title of native window to `title`.

### `win.getTitle()`

Returns the title of the native window.

**Note:** The title of web page can be different from the title of the native
window.

### `win.flashFrame(flag)`

* `flag` Boolean

Starts or stops flashing the window to attract user's attention.

### `win.setSkipTaskbar(skip)`

* `skip` Boolean

Makes the window not show in the taskbar.

### `win.setKiosk(flag)`

* `flag` Boolean

Enters or leaves the kiosk mode.

### `win.isKiosk()`

Returns whether the window is in kiosk mode.

### `win.hookWindowMessage(message, callback)` _WINDOWS_

* `message` Integer
* `callback` Function

Hooks a windows message. The `callback` is called when
the message is received in the WndProc.

### `win.isWindowMessageHooked(message)` _WINDOWS_

* `message` Integer

Returns `true` or `false` depending on whether the message is hooked.

### `win.unhookWindowMessage(message)` _WINDOWS_

* `message` Integer

Unhook the window message.

### `win.unhookAllWindowMessages()` _WINDOWS_

Unhooks all of the window messages.

### `win.setRepresentedFilename(filename)` _OS X_

* `filename` String

Sets the pathname of the file the window represents, and the icon of the file
will show in window's title bar.

### `win.getRepresentedFilename()` _OS X_

Returns the pathname of the file the window represents.

### `win.setDocumentEdited(edited)` _OS X_

* `edited` Boolean

Specifies whether the window’s document has been edited, and the icon in title
bar will become grey when set to `true`.

### `win.isDocumentEdited()` _OS X_

Whether the window's document has been edited.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (optional)- The area of page to be captured, properties:
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

Captures a snapshot of the page within `rect`. Upon completion `callback` will
be called with `callback(image)`. The `image` is an instance of
[NativeImage](native-image.md) that stores data of the snapshot. Omitting
`rect` will capture the whole visible page.

### `win.print([options])`

Same as `webContents.print([options])`

### `win.printToPDF(options, callback)`

Same as `webContents.printToPDF(options, callback)`

### `win.loadURL(url[, options])`

Same as `webContents.loadURL(url[, options])`.

### `win.reload()`

Same as `webContents.reload`.

### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

Sets the `menu` as the window's menu bar, setting it to `null` will remove the
menu bar.

### `win.setProgressBar(progress)`

* `progress` Double

Sets progress value in progress bar. Valid range is [0, 1.0].

Remove progress bar when progress < 0;
Change to indeterminate mode when progress > 1.

On Linux platform, only supports Unity desktop environment, you need to specify
the `*.desktop` file name to `desktopName` field in `package.json`. By default,
it will assume `app.getName().desktop`.

### `win.setOverlayIcon(overlay, description)` _Windows 7+_

* `overlay` [NativeImage](native-image.md) - the icon to display on the bottom
right corner of the taskbar icon. If this parameter is `null`, the overlay is
cleared
* `description` String - a description that will be provided to Accessibility
screen readers

Sets a 16px overlay onto the current taskbar icon, usually used to convey some
sort of application status or to passively notify the user.


### `win.setThumbarButtons(buttons)` _Windows 7+_

`buttons` Array of `button` Objects:

`button` Object, properties:

* `icon` [NativeImage](native-image.md) - The icon showing in thumbnail
  toolbar.
* `tooltip` String (optional) - The text of the button's tooltip.
* `flags` Array (optional) - Control specific states and behaviors
  of the button. By default, it uses `enabled`. It can include following
  Strings:
  * `enabled` - The button is active and available to the user.
  * `disabled` - The button is disabled. It is present, but has a visual
    state indicating it will not respond to user action.
  * `dismissonclick` - When the button is clicked, the taskbar button's
    flyout closes immediately.
  * `nobackground` - Do not draw a button border, use only the image.
  * `hidden` - The button is not shown to the user.
  * `noninteractive` - The button is enabled but not interactive; no
    pressed button state is drawn. This value is intended for instances
    where the button is used in a notification.
* `click` - Function

Add a thumbnail toolbar with a specified set of buttons to the thumbnail image
of a window in a taskbar button layout. Returns a `Boolean` object indicates
whether the thumbnail has been added successfully.

The number of buttons in thumbnail toolbar should be no greater than 7 due to
the limited room. Once you setup the thumbnail toolbar, the toolbar cannot be
removed due to the platform's limitation. But you can call the API with an empty
array to clean the buttons.

### `win.showDefinitionForSelection()` _OS X_

Shows pop-up dictionary that searches the selected word on the page.

### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

Sets whether the window menu bar should hide itself automatically. Once set the
menu bar will only show when users press the single `Alt` key.

If the menu bar is already visible, calling `setAutoHideMenuBar(true)` won't
hide it immediately.

### `win.isMenuBarAutoHide()`

Returns whether menu bar automatically hides itself.

### `win.setMenuBarVisibility(visible)`

* `visible` Boolean

Sets whether the menu bar should be visible. If the menu bar is auto-hide, users
can still bring up the menu bar by pressing the single `Alt` key.

### `win.isMenuBarVisible()`

Returns whether the menu bar is visible.

### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

Sets whether the window should be visible on all workspaces.

**Note:** This API does nothing on Windows.

### `win.isVisibleOnAllWorkspaces()`

Returns whether the window is visible on all workspaces.

**Note:** This API always returns false on Windows.

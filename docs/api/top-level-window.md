# TopLevelWindow

> Create and control native windows.

Process: [Main](../glossary.md#main-process)

```javascript
// In the main process.
const { TopLevelWindow } = require('electron')
const win = new TopLevelWindow({ width: 800, height: 600 })
```

## Class: TopLevelWindow

> Create and control native windows.

Process: [Main](../glossary.md#main-process)

`TopLevelWindow` is an [EventEmitter][event-emitter].

### `new TopLevelWindow(options)` _Experimental_

* `options` Omit<BrowserWindowConstructorOptions, 'webPreferences'>

Creates a new `TopLevelWindow` with native properties as set by the `options`.

### Instance Events

Objects created with `new TopLevelWindow` emit the following events:

#### Event: 'close'

Refs ['close' event of BrowserWindow](./browser-window.md#event-close).

#### Event: 'closed'

Refs ['closed' event of BrowserWindow](./browser-window.md#event-closed).

#### Event: 'session-end' _Windows_

Refs ['session-end' event of BrowserWindow](./browser-window.md#event-session-end).

#### Event: 'blur'

Refs ['blur' event of BrowserWindow](./browser-window.md#event-blur).

#### Event: 'focus'

Refs ['focus' event of BrowserWindow](./browser-window.md#event-focus).

#### Event: 'show'

Refs ['show' event of BrowserWindow](./browser-window.md#event-show).

#### Event: 'hide'

Refs ['hide' event of BrowserWindow](./browser-window.md#event-hide).

#### Event: 'maximize'

Refs ['maximize' event of BrowserWindow](./browser-window.md#event-maximize).

#### Event: 'unmaximize'

Refs ['unmaximize' event of BrowserWindow](./browser-window.md#event-unmaximize).

#### Event: 'minimize'

Refs ['minimize' event of BrowserWindow](./browser-window.md#event-minimize).

#### Event: 'restore'

Refs ['restore' event of BrowserWindow](./browser-window.md#event-restore).

#### Event: 'will-resize' _macOS_ _Windows_

Refs ['will-resize' event of BrowserWindow](./browser-window.md#event-will-resize).

#### Event: 'resize'

Refs ['resize' event of BrowserWindow](./browser-window.md#event-resize).

#### Event: 'will-move' _macOS_ _Windows_

Refs ['will-move' event of BrowserWindow](./browser-window.md#event-will-move).

#### Event: 'move'

Refs ['move' event of BrowserWindow](./browser-window.md#event-move).

#### Event: 'moved' _macOS_

Refs ['moved' event of BrowserWindow](./browser-window.md#event-moved).

#### Event: 'enter-full-screen'

Refs ['enter-full-screen' event of BrowserWindow](./browser-window.md#event-enter-full-screen).

#### Event: 'leave-full-screen'

Refs ['leave-full-screen' event of BrowserWindow](./browser-window.md#event-leave-full-screen).

#### Event: 'always-on-top-changed'

Refs ['always-on-top-changed' event of BrowserWindow](./browser-window.md#event-always-on-top-changed).

#### Event: 'app-command' _Windows_ _Linux_

Refs ['app-command' event of BrowserWindow](./browser-window.md#event-app-command).

#### Event: 'scroll-touch-begin' _macOS_

Refs ['scroll-touch-begin' event of BrowserWindow](./browser-window.md#event-scroll-touch-begin).

#### Event: 'scroll-touch-end' _macOS_

Refs ['scroll-touch-end' event of BrowserWindow](./browser-window.md#event-scroll-touch-end).

#### Event: 'swipe' _macOS_

Refs ['swipe' event of BrowserWindow](./browser-window.md#event-swipe).

#### Event: 'rotate-gesture' _macOS_

Refs ['rotate-gesture' event of BrowserWindow](./browser-window.md#event-rotate-gesture).

#### Event: 'sheet-begin' _macOS_

Refs ['sheet-begin' event of BrowserWindow](./browser-window.md#event-sheet-begin).

#### Event: 'sheet-end' _macOS_

Refs ['sheet-end' event of BrowserWindow](./browser-window.md#event-sheet-end).

#### Event: 'new-window-for-tab' _macOS_

Refs ['new-window-for-tab' event of BrowserWindow](./browser-window.md#event-new-window-for-tab).

### Static Methods

The `TopLevelWindow` class has the following static methods:

#### `TopLevelWindow.getAllWindows()`

Returns `TopLevelWindow[]` - An array of all opened windows.

#### `TopLevelWindow.getFocusedWindow()`

Returns `TopLevelWindow | null` - The window that is focused in this
application, otherwise returns `null`.

#### `TopLevelWindow.fromId(id)`

* `id` Integer

Returns `TopLevelWindow` - The window with the given `id`.

### Instance Properties

Objects created with `new TopLevelWindow` have the following properties:

#### `win.id` _Readonly_

Refs [`BrowserWindow.id`](./browser-window.md#winid).

#### `win.autoHideMenuBar`

Refs [`BrowserWindow.autoHideMenuBar`](./browserwindow.md#winautohidemenubar).

#### `win.simpleFullScreen`

Refs [`BrowserWindow.simpleFullScreen`](./browserwindow.md#winsimplefullscreen).

#### `win.fullScreen`

Refs [`BrowserWindow.fullScreen`](./browserwindow.md#winfullscreen).

#### `win.visibleOnAllWorkspaces`

Refs [`BrowserWindow.visibleOnAllWorkspaces`](./browserwindow.md#winvisibleonallworkspaces).

#### `win.shadow`

Refs [`BrowserWindow.shadow`](./browserwindow.md#winshadow).

#### `win.menuBarVisible` _Windows_ _Linux_

Refs [`BrowserWindow.menuBarVisible`](./browserwindow.md#winmenubarvisible).

####  `win.kiosk`

Refs [`BrowserWindow.kiosk`](./browserwindow.md#winkiosk).

#### `win.documentEdited` _macOS_

Refs [`BrowserWindow.documentEdited`](./browserwindow.md#windocumentedited).

#### `win.representedFilename` _macOS_

Refs [`BrowserWindow.representedFilename`](./browserwindow.md#winrepresentedfilename).

#### `win.title`

Refs [`BrowserWindow.title`](./browserwindow.md#wintitle).

#### `win.minimizable`

Refs [`BrowserWindow.minimizable`](./browserwindow.md#winminimizable).

#### `win.maximizable`

Refs [`BrowserWindow.maximizable`](./browserwindow.md#winmaximizable).

#### `win.fullScreenable`

Refs [`BrowserWindow.fullScreenable`](./browserwindow.md#winfullscreenable).

#### `win.resizable`

Refs [`BrowserWindow.resizable`](./browserwindow.md#winresizable).

#### `win.closable`

Refs [`BrowserWindow.closable`](./browserwindow.md#winclosable).

#### `win.movable`

Refs [`BrowserWindow.movable`](./browserwindow.md#winmovable).

#### `win.excludedFromShownWindowsMenu` _macOS_

Refs [`BrowserWindow.excludedFromShownWindowsMenu`](./browserwindow.md#winexcludedfromshownwindowsmenu).

#### `win.accessibleTitle`

Refs [`BrowserWindow.accessibleTitle`](./browserwindow.md#winaccessibletitle).

### Instance Methods

Objects created with `new TopLevelWindow` have the following instance methods:

#### `win.destroy()`

Refs [BrowserWindow.destroy](./browser-window.md#windestroy).

#### `win.close()`

Refs [BrowserWindow.close](./browser-window.md#winclose).

#### `win.focus()`

Refs [BrowserWindow.focus](./browser-window.md#winfocus).

#### `win.blur()`

Refs [BrowserWindow.blur](./browser-window.md#winblur).

#### `win.isFocused()`

Refs [BrowserWindow.isFocused](./browser-window.md#winisfocused).

#### `win.isDestroyed()`

Refs [BrowserWindow.isDestroyed](./browser-window.md#winisdestroyed).

#### `win.show()`

Refs [BrowserWindow.show](./browser-window.md#winshow).

#### `win.showInactive()`

Refs [BrowserWindow.showInactive](./browser-window.md#winshowinactive).

#### `win.hide()`

Refs [BrowserWindow.hide](./browser-window.md#winhide).

#### `win.isVisible()`

Refs [BrowserWindow.isVisible](./browser-window.md#winisvisible).

#### `win.isModal()`

Refs [BrowserWindow.isModal](./browser-window.md#winismodal).

#### `win.maximize()`

Refs [BrowserWindow.maximize](./browser-window.md#winmaximize).

#### `win.unmaximize()`

Refs [BrowserWindow.unmaximize](./browser-window.md#winunmaximize).

#### `win.isMaximized()`

Refs [BrowserWindow.isMaximized](./browser-window.md#winismaximized).

#### `win.minimize()`

Refs [BrowserWindow.minimize](./browser-window.md#winminimize).

#### `win.restore()`

Refs [BrowserWindow.restore](./browser-window.md#winrestore).

#### `win.isMinimized()`

Refs [BrowserWindow.isMinimized](./browser-window.md#winisminimized).

#### `win.setFullScreen(flag)`

Refs [BrowserWindow.setFullScreen](./browser-window.md#winsetfullscreen).

#### `win.isFullScreen()`

Refs [BrowserWindow.isFullScreen](./browser-window.md#winisfullscreen).

#### `win.setSimpleFullScreen(flag)` _macOS_

Refs [BrowserWindow.setSimpleFullScreen](./browser-window.md#winsetsimplefullscreen).

#### `win.isSimpleFullScreen()` _macOS_

Refs [BrowserWindow.isSimpleFullScreen](./browser-window.md#winissimplefullscreen).

#### `win.isNormal()`

Refs [BrowserWindow.isNormal](./browser-window.md#winisnormal).

#### `win.setAspectRatio(aspectRatio[, extraSize])` _macOS_ _Linux_

Refs [BrowserWindow.setAspectRatio](./browser-window.md#winsetaspectratio).

#### `win.setBackgroundColor(backgroundColor)`

Refs [BrowserWindow.setBackgroundColor](./browser-window.md#winsetbackgroundcolor).

#### `win.previewFile(path[, displayName])` _macOS_

Refs [BrowserWindow.previewFile](./browser-window.md#winpreviewfile).

#### `win.closeFilePreview()` _macOS_

Refs [BrowserWindow.closeFilePreview](./browser-window.md#winclosefilepreview).

#### `win.setBounds(bounds[, animate])`

Refs [BrowserWindow.setBounds](./browser-window.md#winsetbounds).

#### `win.getBounds()`

Refs [BrowserWindow.getBounds](./browser-window.md#wingetbounds).

#### `win.getBackgroundColor()`

Refs [BrowserWindow.getBackgroundColor](./browser-window.md#wingetbackgroundcolor).

#### `win.setContentBounds(bounds[, animate])`

Refs [BrowserWindow.setContentBounds](./browser-window.md#winsetcontentbounds).

#### `win.getContentBounds()`

Refs [BrowserWindow.getContentBounds](./browser-window.md#wingetcontentbounds).

#### `win.getNormalBounds()`

Refs [BrowserWindow.getNormalBounds](./browser-window.md#wingetnormalbounds).

#### `win.setEnabled(enable)`

Refs [BrowserWindow.setEnabled](./browser-window.md#winsetenabled).

#### `win.isEnabled()`

Refs [BrowserWindow.isEnabled](./browser-window.md#winisenabled).

#### `win.setSize(width, height[, animate])`

Refs [BrowserWindow.setSize](./browser-window.md#winsetsize).

#### `win.getSize()`

Refs [BrowserWindow.getSize](./browser-window.md#wingetsize).

#### `win.setContentSize(width, height[, animate])`

Refs [BrowserWindow.setContentSize](./browser-window.md#winsetcontentsize).

#### `win.getContentSize()`

Refs [BrowserWindow.getContentSize](./browser-window.md#wingetcontentsize).

#### `win.setMinimumSize(width, height)`

Refs [BrowserWindow.setMinimumSize](./browser-window.md#winsetminimumsize).

#### `win.getMinimumSize()`

Refs [BrowserWindow.getMinimumSize](./browser-window.md#wingetminimumsize).

#### `win.setMaximumSize(width, height)`

Refs [BrowserWindow.setMaximumSize](./browser-window.md#winsetmaximumsize).

#### `win.getMaximumSize()`

Refs [BrowserWindow.getMaximumSize](./browser-window.md#wingetmaximumsize).

#### `win.setResizable(resizable)`

Refs [BrowserWindow.setResizable](./browser-window.md#winsetresizable).

#### `win.isResizable()`

Refs [BrowserWindow.isResizable](./browser-window.md#winisresizable).

#### `win.setMovable(movable)` _macOS_ _Windows_

Refs [BrowserWindow.setMovable](./browser-window.md#winsetmovable).

#### `win.isMovable()` _macOS_ _Windows_

Refs [BrowserWindow.isMovable](./browser-window.md#winismovable).

#### `win.setMinimizable(minimizable)` _macOS_ _Windows_

Refs [BrowserWindow.setMinimizable](./browser-window.md#winsetminimizable).

#### `win.isMinimizable()` _macOS_ _Windows_

Refs [BrowserWindow.isMinimizable](./browser-window.md#winisminimizable).

#### `win.setMaximizable(maximizable)` _macOS_ _Windows_

Refs [BrowserWindow.setMaximizable](./browser-window.md#winsetmaximizable).

#### `win.isMaximizable()` _macOS_ _Windows_

Refs [BrowserWindow.isMaximizable](./browser-window.md#winismaximizable).

#### `win.setFullScreenable(fullscreenable)`

Refs [BrowserWindow.setFullScreenable](./browser-window.md#winsetfullscreenable).

#### `win.isFullScreenable()`

Refs [BrowserWindow.isFullScreenable](./browser-window.md#winisfullscreenable).

#### `win.setClosable(closable)` _macOS_ _Windows_

Refs [BrowserWindow.setClosable](./browser-window.md#winsetclosable).

#### `win.isClosable()` _macOS_ _Windows_

Refs [BrowserWindow.isClosable](./browser-window.md#winisclosable).

#### `win.setAlwaysOnTop(flag[, level][, relativeLevel])`

Refs [BrowserWindow.setAlwaysOnTop](./browser-window.md#winsetalwaysontop).

#### `win.isAlwaysOnTop()`

Refs [BrowserWindow.isAlwaysOnTop](./browser-window.md#winisalwaysontop).

#### `win.moveAbove(mediaSourceId)`

Refs [BrowserWindow.moveAbove](./browser-window.md#winmoveabove).

#### `win.moveTop()`

Refs [BrowserWindow.moveTop](./browser-window.md#winmovetop).

#### `win.center()`

Refs [BrowserWindow.center](./browser-window.md#wincenter).

#### `win.setPosition(x, y[, animate])`

Refs [BrowserWindow.setPosition](./browser-window.md#winsetposition).

#### `win.getPosition()`

Refs [BrowserWindow.getPosition](./browser-window.md#wingetposition).

#### `win.setTitle(title)`

Refs [BrowserWindow.setTitle](./browser-window.md#winsettitle).

#### `win.getTitle()`

Refs [BrowserWindow.getTitle](./browser-window.md#wingettitle).

#### `win.setSheetOffset(offsetY[, offsetX])` _macOS_

Refs [BrowserWindow.setSheetOffset](./browser-window.md#winsetsheetoffset).

#### `win.flashFrame(flag)`

Refs [BrowserWindow.flashFrame](./browser-window.md#winflashframe).

#### `win.setSkipTaskbar(skip)`

Refs [BrowserWindow.setSkipTaskbar](./browser-window.md#winsetskiptaskbar).

#### `win.setKiosk(flag)`

Refs [BrowserWindow.setKiosk](./browser-window.md#winsetkiosk).

#### `win.isKiosk()`

Refs [BrowserWindow.isKiosk](./browser-window.md#winiskiosk).

#### `win.getMediaSourceId()`

Refs [BrowserWindow.getMediaSourceId](./browser-window.md#wingetmediasourceid).

#### `win.getNativeWindowHandle()`

Refs [BrowserWindow.getNativeWindowHandle](./browser-window.md#wingetnativewindowhandle).

#### `win.hookWindowMessage(message, callback)` _Windows_

Refs [BrowserWindow.hookWindowMessage](./browser-window.md#winhookwindowmessage).

#### `win.isWindowMessageHooked(message)` _Windows_

Refs [BrowserWindow.isWindowMessageHooked](./browser-window.md#winiswindowmessagehooked).

#### `win.unhookWindowMessage(message)` _Windows_

Refs [BrowserWindow.unhookWindowMessage](./browser-window.md#winunhookwindowmessage).

#### `win.unhookAllWindowMessages()` _Windows_

Refs [BrowserWindow.unhookAllWindowMessages](./browser-window.md#winunhookallwindowmessages).

#### `win.setRepresentedFilename(filename)` _macOS_

Refs [BrowserWindow.setRepresentedFilename](./browser-window.md#winsetrepresentedfilename).

#### `win.getRepresentedFilename()` _macOS_

Refs [BrowserWindow.getRepresentedFilename](./browser-window.md#wingetrepresentedfilename).

#### `win.setDocumentEdited(edited)` _macOS_

Refs [BrowserWindow.setDocumentEdited](./browser-window.md#winsetdocumentedited).

#### `win.isDocumentEdited()` _macOS_

Refs [BrowserWindow.isDocumentEdited](./browser-window.md#winisdocumentedited).

#### `win.setMenu(menu)` _Linux_ _Windows_

Refs [BrowserWindow.setMenu](./browser-window.md#winsetmenu).

#### `win.removeMenu()` _Linux_ _Windows_

Refs [BrowserWindow.removeMenu](./browser-window.md#winremovemenu).

#### `win.setProgressBar(progress[, options])`

Refs [BrowserWindow.setProgressBar](./browser-window.md#winsetprogressbar).

#### `win.setOverlayIcon(overlay, description)` _Windows_

Refs [BrowserWindow.setOverlayIcon](./browser-window.md#winsetoverlayicon).

#### `win.setHasShadow(hasShadow)`

Refs [BrowserWindow.setHasShadow](./browser-window.md#winsethasshadow).

#### `win.hasShadow()`

Refs [BrowserWindow.hasShadow](./browser-window.md#winhasshadow).

#### `win.setOpacity(opacity)` _Windows_ _macOS_

Refs [BrowserWindow.setOpacity](./browser-window.md#winsetopacity).

#### `win.getOpacity()`

Refs [BrowserWindow.getOpacity](./browser-window.md#wingetopacity).

#### `win.setShape(rects)` _Windows_ _Linux_ _Experimental_

Refs [BrowserWindow.setShape](./browser-window.md#winsetshape).

#### `win.setThumbarButtons(buttons)` _Windows_

Refs [BrowserWindow.setThumbarButtons](./browser-window.md#winsetthumbarbuttons).

#### `win.setThumbnailClip(region)` _Windows_

Refs [BrowserWindow.setThumbnailClip](./browser-window.md#winsetthumbnailclip).

#### `win.setThumbnailToolTip(toolTip)` _Windows_

Refs [BrowserWindow.setThumbnailToolTip](./browser-window.md#winsetthumbnailtooltip).

#### `win.setAppDetails(options)` _Windows_

Refs [BrowserWindow.setAppDetails](./browser-window.md#winsetappdetails).

#### `win.showDefinitionForSelection()` _macOS_

Refs [BrowserWindow.showDefinitionForSelection](./browser-window.md#winshowdefinitionforselection).

#### `win.setIcon(icon)` _Windows_ _Linux_

Refs [BrowserWindow.setIcon](./browser-window.md#winseticon).

#### `win.setWindowButtonVisibility(visible)` _macOS_

Refs [BrowserWindow.setWindowButtonVisibility](./browser-window.md#winsetwindowbuttonvisibility).

#### `win.setAutoHideMenuBar(hide)`

Refs [BrowserWindow.setAutoHideMenuBar](./browser-window.md#winsetautohidemenubar).

#### `win.isMenuBarAutoHide()`

Refs [BrowserWindow.isMenuBarAutoHide](./browser-window.md#winismenubarautohide).

#### `win.setMenuBarVisibility(visible)` _Windows_ _Linux_

Refs [BrowserWindow.setMenuBarVisibility](./browser-window.md#winsetmenubarvisibility).

#### `win.isMenuBarVisible()`

Refs [BrowserWindow.isMenuBarVisible](./browser-window.md#winismenubarvisible).

#### `win.setVisibleOnAllWorkspaces(visible)`

Refs [BrowserWindow.setVisibleOnAllWorkspaces](./browser-window.md#winsetvisibleonallworkspaces).

#### `win.isVisibleOnAllWorkspaces()`

Refs [BrowserWindow.isVisibleOnAllWorkspaces](./browser-window.md#winisvisibleonallworkspaces).

#### `win.setIgnoreMouseEvents(ignore[, options])`

Refs [BrowserWindow.setIgnoreMouseEvents](./browser-window.md#winsetignoremouseevents).

#### `win.setContentProtection(enable)` _macOS_ _Windows_

Refs [BrowserWindow.setContentProtection](./browser-window.md#winsetcontentprotection).

#### `win.setFocusable(focusable)` _macOS_ _Windows_

Refs [BrowserWindow.setFocusable](./browser-window.md#winsetfocusable).

#### `win.setParentWindow(parent)`

Refs [BrowserWindow.setParentWindow](./browser-window.md#winsetparentwindow).

#### `win.getParentWindow()`

Refs [BrowserWindow.getParentWindow](./browser-window.md#wingetparentwindow).

#### `win.getChildWindows()`

Refs [BrowserWindow.getChildWindows](./browser-window.md#wingetchildwindows).

#### `win.setAutoHideCursor(autoHide)` _macOS_

Refs [BrowserWindow.setAutoHideCursor](./browser-window.md#winsetautohidecursor).

#### `win.selectPreviousTab()` _macOS_

Refs [BrowserWindow.selectPreviousTab](./browser-window.md#winselectprevioustab).

#### `win.selectNextTab()` _macOS_

Refs [BrowserWindow.selectNextTab](./browser-window.md#winselectnexttab).

#### `win.mergeAllWindows()` _macOS_

Refs [BrowserWindow.mergeAllWindows](./browser-window.md#winmergeallwindows).

#### `win.moveTabToNewWindow()` _macOS_

Refs [BrowserWindow.moveTabToNewWindow](./browser-window.md#winmovetabtonewwindow).

#### `win.toggleTabBar()` _macOS_

Refs [BrowserWindow.toggleTabBar](./browser-window.md#wintoggletabbar).

#### `win.addTabbedWindow(topLevelWindow)` _macOS_

Refs [BrowserWindow.addTabbedWindow](./browser-window.md#winaddtabbedwindow).

#### `win.setVibrancy(type)` _macOS_

Refs [BrowserWindow.setVibrancy](./browser-window.md#winsetvibrancy).

#### `win.setTrafficLightPosition(position)` _macOS_

Refs [BrowserWindow.setTrafficLightPosition](./browser-window.md#winsettrafficlightposition).

#### `win.getTrafficLightPosition()` _macOS_

Refs [BrowserWindow.getTrafficLightPosition](./browser-window.md#wingettrafficlightposition).

#### `win.setTouchBar(touchBar)` _macOS_

Refs [BrowserWindow.setTouchBar](./browser-window.md#winsettouchbar).

#### `win.setBrowserView(browserView)` _Experimental_

Refs [BrowserWindow.setBrowserView](./browser-window.md#winsetbrowserview).

#### `win.getBrowserView()` _Experimental_

Refs [BrowserWindow.getBrowserView](./browser-window.md#wingetbrowserview).

#### `win.addBrowserView(browserView)` _Experimental_

Refs [BrowserWindow.addBrowserView](./browser-window.md#winaddbrowserview).

#### `win.removeBrowserView(browserView)` _Experimental_

Refs [BrowserWindow.removeBrowserView](./browser-window.md#winremovebrowserview).

#### `win.getBrowserViews()` _Experimental_

Refs [BrowserWindow.getBrowserViews](./browser-window.md#wingetbrowserviews).

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter

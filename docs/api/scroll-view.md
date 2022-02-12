# ScrollView

Show a part of view with scrollbar. 
The `ScrollView` can show an arbitrary content view inside it. It is used to make
any View scrollable. When the content is larger than the `ScrollView`,
scrollbars will be optionally showed. When the content view is smaller
then the `ScrollView`, the content view will be resized to the size of the
`ScrollView`.
The scrollview supports keyboard UI and mousewheel.
It extends [`BaseView`](base-view.md).

## Class: ScrollView  extends `BaseView`

> Create and control scroll views.

Process: [Main](../glossary.md#main-process)

### Example

```javascript
// In the main process.
const path = require('path')
const { app, BrowserView, BaseWindow, ContainerView, ScrollView, WrapperBrowserView } = require('electron')

const WEBVIEW_WIDTH = 600
const GAP = 30
const URLS = [
  'https://theverge.com/',
  'https://mashable.com',
  'https://www.businessinsider.com'
]

global.win = null

app.whenReady().then(() => {
  win = new BaseWindow({ autoHideMenuBar: true, width: 1400, height: 700 })

  // The content view.
  const contentView = new ContainerView()
  contentView.setBackgroundColor('#1F2937')
  win.setContentBaseView(contentView)
  contentView.setBounds({ x: 0, y: 0, width: 1378, height: 600 })

  // Scroll
  const scroll = new ScrollView()
  scroll.setHorizontalScrollBarMode('enabled')
  scroll.setVerticalScrollBarMode('disabled')
  contentView.addChildView(scroll)
  scroll.setBounds({ x: 0, y: 0, width: 1377, height: 600 })

  // Scroll content
  const scrollContent = new ContainerView()
  scroll.setContentView(scrollContent)
  scrollContent.setBounds({ x: 0, y: 0, width: URLS.length * (WEBVIEW_WIDTH + GAP), height: 600 })

  for (var i = 1; i <= URLS.length; i++) {
    const browserView = new BrowserView()
    browserView.setBackgroundColor('#ffffff')
    const wrapperBrowserView = new WrapperBrowserView({ browserView })
    const webContentView = new ContainerView()
    webContentView.addChildView(wrapperBrowserView)
    wrapperBrowserView.setBounds({ x: 0, y: 0, width: 600, height: 540 })
    scrollContent.addChildView(webContentView)
    webContentView.setBounds({ x: i * (WEBVIEW_WIDTH + GAP) + GAP, y: 30, width: 600, height: 540 })
    browserView.webContents.loadURL(URLS[i - 1])
  }
})
```

### `new ScrollView()` _Experimental_

Creates the new scroll view.

### Static Methods

The `ScrollView` class has the following static methods:

#### `ScrollView.getAllViews()`

Returns `ScrollView[]` - An array of all created scroll views.

#### `ScrollView.fromId(id)`

* `id` Integer

Returns `ScrollView | null` - The scroll view with the given `id`.

### Instance Methods

Objects created with `new ScrollView` have the following instance methods:

#### `view.setContentView(contents)` _Experimental_

* `contents` [BaseView](base-view.md)

Set the contents. The contents is the view that needs to scroll.

  #### `view.getContentView()` _Experimental_

Returns [`BaseView`](base-view.md) - The contents of the `view`.

#### `view.setHorizontalScrollBarMode(mode)` _Experimental_

* `mode` string - Can be one of the following values: `disabled`, `hidden-but-enabled`, `enabled`. Default is `enabled`.

Controls how the horizontal scroll bar appears and functions.
* `disable` - The scrollbar is hidden, and the pane will not respond to e.g. mousewheel events even if the contents are larger than the viewport.
* `hidden-but-enabled` - The scrollbar is hidden whether or not the contents are larger than the viewport, but the pane will respond to scroll events.
*`enabled` - The scrollbar will be visible if the contents are larger than the viewport and the pane will respond to scroll events.

#### `view.getHorizontalScrollBarMode()` _Experimental_

Returns `string` - horizontal scrollbar mode.

#### `view.setVerticalScrollBarMode(mode)` _Experimental_

* `mode` string - Can be one of the following values: `disabled`, `hidden-but-enabled`, `enabled`. Default is `enabled`.

Controls how the vertical scroll bar appears and functions.
* `disable` - The scrollbar is hidden, and the pane will not respond to e.g. mousewheel events even if the contents are larger than the viewport.
* `hidden-but-enabled` - The scrollbar is hidden whether or not the contents are larger than the viewport, but the pane will respond to scroll events.
*`enabled` - The scrollbar will be visible if the contents are larger than the viewport and the pane will respond to scroll events.

#### `view.getVerticalScrollBarMode()` _Experimental_

Returns `string` - vertical scrollbar mode.

#### `view.setHorizontalScrollElasticity(elasticity)` _macOS_ _Experimental_

* `elasticity` string - Can be one of the following values: `automatic`, `none`, `allowed`. Default is `automatic`.

The scroll view’s horizontal scrolling elasticity mode.
A scroll view can scroll its contents past its bounds to achieve an elastic effect. 
When set to `automatic`, scrolling the horizontal axis beyond its document
bounds only occurs if the document width is greater than the view width,
or the vertical scroller is hidden and the horizontal scroller is visible.
* `automatic` - Automatically determine whether to allow elasticity on this axis.
* `none` - Disallow scrolling beyond document bounds on this axis.
*`allowed` - Allow content to be scrolled past its bounds on this axis in an elastic fashion.

#### `view.getHorizontalScrollElasticity()` _macOS_ _Experimental_

Returns `string` - The scroll view’s horizontal scrolling elasticity mode.

#### `view.setVerticalScrollElasticity(elasticity)` _macOS_ _Experimental_

* `elasticity` string - Can be one of the following values: `automatic`, `none`, `allowed`. Default is `automatic`.

The scroll view’s vertical scrolling elasticity mode.

#### `view.getVerticalScrollElasticity()` _macOS_ _Experimental_

Returns `string` - The scroll view’s vertical scrolling elasticity mode.

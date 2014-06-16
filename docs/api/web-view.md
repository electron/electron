# web-view

The `web-view` module can custom the rendering of current web page.

An example of zooming current page to 200%.

```javascript
var webView = require('web-view');
webView.setZoomFactor(2);
```

## webView.setZoomFactor(factor)

* `factor` Number - Zoom factor

Changes the zoom factor to the specified factor, zoom factor is
zoom percent / 100, so 300% = 3.0.

## webView.getZoomFactor()

Returns the current zoom factor.

## webView.setZoomLevel(level)

* `level` Number - Zoom level

Changes the zoom level to the specified level, 0 is "original size", and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively.

## webView.getZoomLevel()

Returns the current zoom level.

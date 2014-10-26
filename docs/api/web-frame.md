# web-frame

The `web-frame` module can custom the rendering of current web page.

An example of zooming current page to 200%.

```javascript
var webFrame = require('web-frame');
webFrame.setZoomFactor(2);
```

## webFrame.setZoomFactor(factor)

* `factor` Number - Zoom factor

Changes the zoom factor to the specified factor, zoom factor is
zoom percent / 100, so 300% = 3.0.

## webFrame.getZoomFactor()

Returns the current zoom factor.

## webFrame.setZoomLevel(level)

* `level` Number - Zoom level

Changes the zoom level to the specified level, 0 is "original size", and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively.

## webFrame.getZoomLevel()

Returns the current zoom level.

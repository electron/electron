# Display Object

* `accelerometerSupport` string - Can be `available`, `unavailable`, `unknown`.
* `bounds` [Rectangle](rectangle.md) - the bounds of the display in DIP points.
* `colorDepth` number - The number of bits per pixel.
* `colorSpace` string -  represent a color space (three-dimensional object which contains all realizable color combinations) for the purpose of color conversions.
* `depthPerComponent` number - The number of bits per color component.
* `detected` boolean - `true`` if the display is detected by the system.
* `displayFrequency` number - The display refresh rate.
* `id` number - Unique identifier associated with the display. A value of of -1 means the display is invalid or the correct `id` is not yet known, and a value of -10 means the display is a virtual display assigned to a unified desktop.
* `internal` boolean - `true` for an internal display and `false` for an external display.
* `label` string - User-friendly label, determined by the platform.
* `maximumCursorSize` [Size](size.md) - Maximum cursor size in native pixels.
* `nativeOrigin` [Point](point.md) - Returns the display's origin in pixel coordinates. Only available on windowing systems like X11 that position displays in pixel coordinates.
* `rotation` number - Can be 0, 90, 180, 270, represents screen rotation in
  clock-wise degrees.
* `scaleFactor` number - Output device's pixel scale factor.
* `touchSupport` string - Can be `available`, `unavailable`, `unknown`.
* `monochrome` boolean - Whether or not the display is a monochrome display.
* `size` [Size](size.md)
* `workArea` [Rectangle](rectangle.md) - the work area of the display in DIP points.
* `workAreaSize` [Size](size.md) - The size of the work area.

The `Display` object represents a physical display connected to the system. A
fake `Display` may exist on a headless system, or a `Display` may correspond to
a remote, virtual display.

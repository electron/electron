# Display Object

* `id` number - Unique identifier associated with the display.
* `rotation` number - Can be 0, 90, 180, 270, represents screen rotation in
  clock-wise degrees.
* `scaleFactor` number - Output device's pixel scale factor.
* `touchSupport` string - Can be `available`, `unavailable`, `unknown`.
* `monochrome` boolean - Whether or not the display is a monochrome display.
* `accelerometerSupport` string - Can be `available`, `unavailable`, `unknown`.
* `colorSpace` string -  represent a color space (three-dimensional object which contains all realizable color combinations) for the purpose of color conversions
* `colorDepth` number - The number of bits per pixel.
* `depthPerComponent` number - The number of bits per color component.
* `displayFrequency` number - The display refresh rate.
* `bounds` [Rectangle](rectangle.md) - the bounds of the display in DIP points.
* `size` [Size](size.md)
* `workArea` [Rectangle](rectangle.md) - the work area of the display in DIP points.
* `workAreaSize` [Size](size.md)
* `internal` boolean - `true` for an internal display and `false` for an external display

The `Display` object represents a physical display connected to the system. A
fake `Display` may exist on a headless system, or a `Display` may correspond to
a remote, virtual display.

# Display

* `id` Number - Unique identifier associated with the display.
* `rotation` Number - Can be 0, 90, 180, 270, represents screen rotation in
  clock-wise degrees.
* `scaleFactor` Number - Output device's pixel scale factor.
* `touchSupport` String - Can be `available`, `unavailable`, `unknown`.
* `bounds` [Bounds](bounds.md)
* `size` Object
  * `height` Number
  * `width` Number
* `workArea` [Bounds](bounds.md)
* `workAreaSize` Object
  * `height` Number
  * `width` Number

The `Display` object represents a physical display connected to the system. A
fake `Display` may exist on a headless system, or a `Display` may correspond to
a remote, virtual display.

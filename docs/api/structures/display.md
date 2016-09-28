# Display

* `id` Integer - Unique identifier associated with the display.
* `rotation` Integer - Can be 0, 90, 180, 270, represents screen rotation in
  clock-wise degrees.
* `scaleFactor` Number - Output device's pixel scale factor.
* `touchSupport` String - Can be `available`, `unavailable`, `unknown`.
* `bounds` Object
* `size` Object
* `workArea` Object
* `workAreaSize` Object

The `Display` object represents a physical display connected to the system. A
fake `Display` may exist on a headless system, or a `Display` may correspond to
a remote, virtual display.

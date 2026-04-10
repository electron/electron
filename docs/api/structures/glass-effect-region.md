# GlassEffectRegion Object

* `x` number - The x coordinate of the region's origin in window content coordinates.
* `y` number - The y coordinate of the region's origin in window content coordinates.
* `width` number - The width of the region.
* `height` number - The height of the region.
* `style` string (optional) - The glass style. Can be `regular` or `clear`. Defaults to `regular`.
* `cornerRadius` number (optional) - Corner radius of the glass shape. Defaults to `0`.
* `tintColor` string (optional) - Color used to tint the glass, as a CSS string. Hex values with alpha follow Electron's `#AARRGGBB` convention (same as `backgroundColor`). Defaults to no tint.
* `contentImage` [NativeImage](../native-image.md) (optional) - An image to render inside the glass's content view, on top of the glass surface. The image is centered, scaled down proportionally to fit, and receives native vibrancy filtering. Only honoured by `setGlassEffectRegions`.

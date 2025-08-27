# Multi-Monitor Testing

The `virtualDisplay` addon leverages macOS CoreGraphics APIs to create virtual displays, allowing you to write and run multi-monitor tests without the need for physical monitors.

## Methods

#### `virtualDisplay.create([options])`

Creates a virtual display and returns a display ID.

```js @ts-nocheck
const virtualDisplay = require('@electron-ci/virtual-display')
// Default: 1920×1080 at origin (0, 0)
const displayId = virtualDisplay.create()
```

```js @ts-nocheck
const virtualDisplay = require('@electron-ci/virtual-display')
// Custom options (all parameters optional and have default values)
const displayId = virtualDisplay.create({
  width: 2560, // Display width in pixels
  height: 1440, // Display height in pixels
  x: 1920, // X position (top-left corner)
  y: 0 // Y position (top-left corner)
})
```

**Returns:** `number` - Unique display ID used to identify the display. Returns `0` on failure to create display.

#### `virtualDisplay.destroy(displayId)`

Removes the virtual display.

```js @ts-nocheck
const success = virtualDisplay.destroy(displayId)
```

**Returns:** `boolean` - Success status

#### `virtualDisplay.forceCleanup()`

Performs a complete cleanup of all virtual displays and resets the macOS CoreGraphics display system.

It is recommended to call this before every test to prevent test failures. macOS CoreGraphics maintains an internal display ID allocation pool that can become corrupted when virtual displays are created and destroyed rapidly during testing. Without proper cleanup, subsequent display creation may fail with inconsistent display IDs, resulting in test flakiness.

```js @ts-nocheck
// Recommended test pattern
beforeEach(() => {
  virtualDisplay.forceCleanup()
})
```

**Returns:** `boolean` - Success status

## Display Constraints

### Size Limits

Virtual displays are constrained to 720×720 pixels minimum and 8192×8192 pixels maximum. Actual limits may vary depending on your Mac's graphics capabilities, so sizes outside this range (like 9000×6000) may fail on some systems.

```js @ts-nocheck
// Safe sizes for testing
virtualDisplay.create({ width: 1920, height: 1080 }) // Full HD
virtualDisplay.create({ width: 3840, height: 2160 }) // 4K
```

### Positioning Behavior

macOS maintains a contiguous desktop space by automatically adjusting display positions if there are any overlaps or gaps. In case of either, the placement of the new origin is as close as possible to the requested location, without overlapping or leaving a gap between displays.

**Overlap:**

```js @ts-nocheck
// Requested positions
const display1 = virtualDisplay.create({ x: 0, y: 0, width: 1920, height: 1080 })
const display2 = virtualDisplay.create({ x: 500, y: 0, width: 1920, height: 1080 })

// macOS automatically repositions display2 to x: 1920 to prevent overlap
const actualBounds = screen.getAllDisplays().map(d => d.bounds)
// Result: [{ x: 0, y: 0, width: 1920, height: 1080 },
//          { x: 1920, y: 0, width: 1920, height: 1080 }]
```

**Gap:**

```js @ts-nocheck
// Requested: gap between displays
const display1 = virtualDisplay.create({ width: 1920, height: 1080, x: 0, y: 0 })
const display2 = virtualDisplay.create({ width: 1920, height: 1080, x: 2000, y: 0 })
// macOS snaps display2 to x: 1920 (eliminates 80px gap)
```

> [!NOTE]
> Always verify actual positions with `screen.getAllDisplays()` after creation, as macOS may adjust coordinates from the set values.

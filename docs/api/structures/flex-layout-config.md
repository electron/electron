# FlexLayoutConfig Object

FlexLayout is a native layout manager implemented in Chromium.

For more documentation on Layout Managers, see the [Chromium docs](https://chromium.googlesource.com/chromium/src/+/master/docs/ui/views/overview.md#layout-layout-managers).
For more information on how to configure the FlexLayout manager, see the documentation in [the FlexLayout code](https://source.chromium.org/chromium/chromium/src/+/main:ui/views/layout/flex_layout.h).

* `orientation` string - How the layout is oriented. One of the following:
  * `horizontal` - Lay out child views along a horizontal axis
  * `vertical` - Lay out child views along a vertical axis
* `mainAxisAlignment` string - The alignment of children in the main axis. Defaults to `start`. One of the following:
  * `start`
  * `center`
  * `end`
  * `stretch`
  * `baseline`
* `crossAxisAlignment` string - Cross-axis alignment type. One of the following:
  * `start`
  * `center`
  * `end`
  * `stretch`
  * `baseline`
* `interiorMargin` Object - Spacing between child views and host view border
  * `top` number
  * `bottom` number
  * `left` number
  * `right` number
* `minimumCrossAxisSize` number - The minimum cross axis size for the layout
* `collapseMargins` boolean - Whether adjacent view margins should be collapsed
* `includeHostInsetsInLayout` boolean - Whether to include host insets in the layout. Use when e.g. the host has an
   empty border and you want to treat that empty space as part of the interior margin of the host view. Most useful
   in conjunction with `collapseMargins` so child margins can overlap with the host's insets
* `ignoreDefaultMainAxisMargins` boolean - Whether host |interior_margin| overrides default child margins at the
  leading and trailing edge of the host view
* `flexAllocationOrder` string - one of:
  * `normal`
  * `reverse`

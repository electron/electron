# FlexLayoutConfig Object

FlexLayout is a native layout manager implemented in Chromium.

For more documentation on Layout Managers, see the [Chromium docs](https://chromium.googlesource.com/chromium/src/+/master/docs/ui/views/overview.md#layout-layout-managers).
For more information on how to configure the FlexLayout manager, see the documentation in [the FlexLayout code](https://source.chromium.org/chromium/chromium/src/+/main:ui/views/layout/flex_layout.h).

* `orientation` string (optional) - How the layout is oriented. Defaults to `horizontal` One of the following:
  * `horizontal` - Lay out child views along a horizontal axis
  * `vertical` - Lay out child views along a vertical axis
* `mainAxisAlignment` string (optional) - The alignment of children in the main axis. Defaults to `start`. One of the following:
  * `start`
  * `center`
  * `end`
  * `stretch`
  * `baseline`
* `crossAxisAlignment` string (optional) - Cross-axis alignment type. Defaults to `stretch`. One of the following:
  * `start`
  * `center`
  * `end`
  * `stretch`
  * `baseline`
* `interiorMargin` Object (optional) - Spacing between child views and host view border. Defaults to `0` margin.
  * `top` number
  * `bottom` number
  * `left` number
  * `right` number
* `minimumCrossAxisSize` number (optional) - The minimum cross axis size for the layout. Defaults to `0`.
* `collapseMargins` boolean (optional) - Whether adjacent view margins should be collapsed. Defaults to `false`.
* `includeHostInsetsInLayout` boolean (optional) - Whether to include host insets in the layout. Use when e.g. the host has an
   empty border and you want to treat that empty space as part of the interior margin of the host view. Most useful
   in conjunction with `collapseMargins` so child margins can overlap with the host's insets. Defaults to `false`.
* `ignoreDefaultMainAxisMargins` boolean - Whether host |interior_margin| overrides default child margins at the
  leading and trailing edge of the host view. Defaults to `false`.
* `flexAllocationOrder` string (optional) - Order in which the host's child views receive their flex allocation. Setting to
  reverse is useful when, for example, you want views to drop out left-to-right when there's insufficient space to
  display them all instead of right-to-left. Defaults to `normal`. One of the following:
  * `normal`
  * `reverse`

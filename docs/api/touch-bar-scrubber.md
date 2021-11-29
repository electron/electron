## Class: TouchBarScrubber

> Create a scrubber (a scrollable selector)

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### `new TouchBarScrubber(options)`

* `options` Object
  * `items` [ScrubberItem[]](structures/scrubber-item.md) - An array of items to place in this scrubber.
  * `select` Function (optional) - Called when the user taps an item that was not the last tapped item.
    * `selectedIndex` Integer - The index of the item the user selected.
  * `highlight` Function (optional) - Called when the user taps any item.
    * `highlightedIndex` Integer - The index of the item the user touched.
  * `selectedStyle` string (optional) - Selected item style. Can be `background`, `outline` or `none`. Defaults to `none`.
  * `overlayStyle` string (optional) - Selected overlay item style. Can be `background`, `outline` or `none`. Defaults to `none`.
  * `showArrowButtons` boolean (optional) - Whether to show arrow buttons. Defaults to `false` and is only shown if `items` is non-empty.
  * `mode` string (optional) - Can be `fixed` or `free`. The default is `free`.
  * `continuous` boolean (optional) - Defaults to `true`.

### Instance Properties

The following properties are available on instances of `TouchBarScrubber`:

#### `touchBarScrubber.items`

A `ScrubberItem[]` array representing the items in this scrubber. Updating this value immediately
updates the control in the touch bar. Updating deep properties inside this array **does not update the touch bar**.

#### `touchBarScrubber.selectedStyle`

A `string` representing the style that selected items in the scrubber should have. Updating this value immediately
updates the control in the touch bar. Possible values:

* `background` - Maps to `[NSScrubberSelectionStyle roundedBackgroundStyle]`.
* `outline` - Maps to `[NSScrubberSelectionStyle outlineOverlayStyle]`.
* `none` - Removes all styles.

#### `touchBarScrubber.overlayStyle`

A `string` representing the style that selected items in the scrubber should have. This style is overlayed on top
of the scrubber item instead of being placed behind it. Updating this value immediately updates the control in the
touch bar. Possible values:

* `background` - Maps to `[NSScrubberSelectionStyle roundedBackgroundStyle]`.
* `outline` - Maps to `[NSScrubberSelectionStyle outlineOverlayStyle]`.
* `none` - Removes all styles.

#### `touchBarScrubber.showArrowButtons`

A `boolean` representing whether to show the left / right selection arrows in this scrubber. Updating this value
immediately updates the control in the touch bar.

#### `touchBarScrubber.mode`

A `string` representing the mode of this scrubber. Updating this value immediately
updates the control in the touch bar. Possible values:

* `fixed` - Maps to `NSScrubberModeFixed`.
* `free` - Maps to `NSScrubberModeFree`.

#### `touchBarScrubber.continuous`

A `boolean` representing whether this scrubber is continuous or not. Updating this value immediately
updates the control in the touch bar.

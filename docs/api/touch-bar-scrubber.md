## Class: TouchBarScrubber

> Create a scrubber (a scrollablbe selector)

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarScrubber(options)`

* `options` Object
  * `items` [ScrubberItem[]](structures/scrubber-item.md) - An array of items to place in this scruber
  * `onSelect` Function - Called when the user taps an item that was not the last tapped item
    * `selectedIndex` - The index of the item the user selected
  * `onHightlight` Function - Called when the user taps any item
    * `highlightedIndex` - The index of the item the user touched

### Instance Properties

The following properties are available on instances of `TouchBarScrubber`:

#### `touchBarSegmentedControl.items`

A `ScrubberItem[]` array representing the items in this scrubber.  Updating this value immediately
updates the control in the touch bar.  Updating deep properties inside this array **does not update the touch bar**.

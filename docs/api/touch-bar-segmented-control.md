## Class: TouchBarSegmentedControl

> Create a segmented control (a button group) where one button has a selected state

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarSegmentedControl(options)` _Experimental_

* `options` Object
  * `segmentStyle` String - (Optional) Style of the segments:
    * `automatic` - Default
    * `rounded`
    * `textured-rounded`
    * `round-rect`
    * `textured-square`
    * `capsule`
    * `small-square`
    * `separated`
  * `segments` [SegmentedControlSegment[]](structures/segmented-control-segment.md) - An array of segments to place in this control
  * `selectedIndex` Integer (Optional) - The index of the currently selected segment, will update automatically with user interaction
  * `change` Function - Called when the user selects a new segment
    * `selectedIndex` - The index of the segment the user selected

### Instance Properties

The following properties are available on instances of `TouchBarSegmentedControl`:

#### `touchBarSegmentedControl.segmentStyle`

A `String` representing the controls current segment style.  Updating this value immediately updates the control
in the touch bar.

#### `touchBarSegmentedControl.segments`

A `SegmentedControlSegment[]` array representing the segments in this control.  Updating this value immediately
updates the control in the touch bar.  Updating deep properties inside this array **does not update the touch bar**.

#### `touchBarSegmentedControl.selectedIndex`

An `Integer` representing the currently selected segment.  Changing this value immediately updates the control
in the touch bar.  User interaction with the touch bar will update this value automatically.

## Class: TouchBarSlider

> Create a slider in the touch bar for native macOS applications

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarSlider(options)` _Experimental_

* `options` Object
  * `label` String (optional) - Label text.
  * `value` Integer (optional) - Selected value.
  * `minValue` Integer (optional) - Minimum value.
  * `maxValue` Integer (optional) - Maximum value.
  * `change` Function (optional) - Function to call when the slider is changed.
    * `newValue` Number - The value that the user selected on the Slider.

### Instance Properties

The following properties are available on instances of `TouchBarSlider`:

#### `touchBarSlider.label`

A `String` representing the slider's current text. Changing this value immediately updates the slider
in the touch bar.

#### `touchBarSlider.value`

A `Number` representing the slider's current value. Changing this value immediately updates the slider
in the touch bar.

#### `touchBarSlider.minValue`

A `Number` representing the slider's current minimum value. Changing this value immediately updates the
slider in the touch bar.

#### `touchBarSlider.maxValue`

A `Number` representing the slider's current maximum value. Changing this value immediately updates the
slider in the touch bar.

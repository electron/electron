## Class: TouchBarSlider

> Create a slider in the touch bar for native macOS applications

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### `new TouchBarSlider(options)`

* `options` Object
  * `label` string (optional) - Label text.
  * `value` Integer (optional) - Selected value.
  * `minValue` Integer (optional) - Minimum value.
  * `maxValue` Integer (optional) - Maximum value.
  * `change` Function (optional) - Function to call when the slider is changed.
    * `newValue` number - The value that the user selected on the Slider.

### Instance Properties

The following properties are available on instances of `TouchBarSlider`:

#### `touchBarSlider.label`

A `string` representing the slider's current text. Changing this value immediately updates the slider
in the touch bar.

#### `touchBarSlider.value`

A `number` representing the slider's current value. Changing this value immediately updates the slider
in the touch bar.

#### `touchBarSlider.minValue`

A `number` representing the slider's current minimum value. Changing this value immediately updates the
slider in the touch bar.

#### `touchBarSlider.maxValue`

A `number` representing the slider's current maximum value. Changing this value immediately updates the
slider in the touch bar.

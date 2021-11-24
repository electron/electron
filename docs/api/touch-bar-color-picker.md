## Class: TouchBarColorPicker

> Create a color picker in the touch bar for native macOS applications

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### `new TouchBarColorPicker(options)`

* `options` Object
  * `availableColors` string[] (optional) - Array of hex color strings to
    appear as possible colors to select.
  * `selectedColor` string (optional) - The selected hex color in the picker,
    i.e `#ABCDEF`.
  * `change` Function (optional) - Function to call when a color is selected.
    * `color` string - The color that the user selected from the picker.

### Instance Properties

The following properties are available on instances of `TouchBarColorPicker`:

#### `touchBarColorPicker.availableColors`

A `string[]` array representing the color picker's available colors to select. Changing this value immediately
updates the color picker in the touch bar.

#### `touchBarColorPicker.selectedColor`

A `string` hex code representing the color picker's currently selected color. Changing this value immediately
updates the color picker in the touch bar.

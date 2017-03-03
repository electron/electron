## Class: TouchBarColorPicker

> Create a color picker in the touch bar for native macOS applications

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarColorPicker(options)`

* `options` Object
  * `availableColors` String[] (optional) - Array of hex color strings to
    appear as possible colors to select.
  * `selectedColor` String (optional) - The selected hex color in the picker,
    i.e `#ABCDEF`.
  * `change` Function (optional) - Function to call when a color is selected.

### Instance Properties

The following properties are available on instances of `TouchBarColorPicker`:

#### `touchBarColorPicker.availableColors`

The color picker's available colors to select. Changing this value immediately
updates the color picker in the touch bar.

#### `touchBarColorPicker.selectedColor`

The color picker's currently selected color. Changing this value immediately
updates the color picker in the touch bar.

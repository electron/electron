## Class: TouchBarLabel

> Create a label in the touch bar for native macOS applications

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarLabel(options)`

* `options` Object
  * `label` String (optional) - Text to display.
  * `textColor` String (optional) - Hex color of text, i.e `#ABCDEF`.

### Instance Properties

The following properties are available on instances of `TouchBarLabel`:

#### `touchBarLabel.label`

The label's current text. Changing this value immediately updates the label in
the touch bar.

#### `touchBarLabel.textColor`

The label's current text color. Changing this value immediately updates the
label in the touch bar.

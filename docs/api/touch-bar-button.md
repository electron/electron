## Class: TouchBarButton

> Create a button in the touch bar for native macOS applications

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarButton(options)`

* `options` Object
  * `label` String (optional) - Button text.
  * `backgroundColor` String (optional) - Button background color in hex format,
    i.e `#ABCDEF`.
  * `icon` NativeImage (optional) - Button icon.
  * `click` Function (optional) - Function to call when the button is clicked.

### Instance Properties

The following properties are available on instances of `TouchBarButton`:

#### `touchBarButton.label`

The button's current text. Changing this value immediately updates the button
in the touch bar.

#### `touchBarButton.backgroundColor`

The button's current background color. Changing this value immediately updates
the button in the touch bar.

#### `touchBarButton.icon`

The button's current icon. Changing this value immediately updates the button
in the touch bar.

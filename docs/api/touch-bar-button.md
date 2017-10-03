## Class: TouchBarButton

> Create a button in the touch bar for native macOS applications

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBarButton(options)` _Experimental_

* `options` Object
  * `label` String (optional) - Button text.
  * `backgroundColor` String (optional) - Button background color in hex format,
    i.e `#ABCDEF`.
  * `icon` [NativeImage](native-image.md) (optional) - Button icon.
  * `iconPosition` String (optional) - Can be `left`, `right` or `overlay`.
  * `click` Function (optional) - Function to call when the button is clicked.

### Instance Properties

The following properties are available on instances of `TouchBarButton`:

#### `touchBarButton.label`

A `String` representing the button's current text. Changing this value immediately updates the button
in the touch bar.

#### `touchBarButton.backgroundColor`

A `String` hex code representing the button's current background color. Changing this value immediately updates
the button in the touch bar.

#### `touchBarButton.icon`

A `NativeImage` representing the button's current icon. Changing this value immediately updates the button
in the touch bar.

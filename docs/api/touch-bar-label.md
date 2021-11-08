## Class: TouchBarLabel

> Create a label in the touch bar for native macOS applications

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### `new TouchBarLabel(options)`

* `options` Object
  * `label` string (optional) - Text to display.
  * `accessibilityLabel` string (optional) - A short description of the button for use by screenreaders like VoiceOver.
  * `textColor` string (optional) - Hex color of text, i.e `#ABCDEF`.

When defining `accessibilityLabel`, ensure you have considered macOS [best practices](https://developer.apple.com/documentation/appkit/nsaccessibilitybutton/1524910-accessibilitylabel?language=objc).

### Instance Properties

The following properties are available on instances of `TouchBarLabel`:

#### `touchBarLabel.label`

A `string` representing the label's current text. Changing this value immediately updates the label in
the touch bar.

#### `touchBarLabel.accessibilityLabel`

A `string` representing the description of the label to be read by a screen reader.

#### `touchBarLabel.textColor`

A `string` hex code representing the label's current text color. Changing this value immediately updates the
label in the touch bar.

## Class: TouchBarSpacer

> Create a spacer between two items in the touch bar for native macOS applications

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### `new TouchBarSpacer(options)`

* `options` Object
  * `size` string (optional) - Size of spacer, possible values are:
    * `small` - Small space between items. Maps to `NSTouchBarItemIdentifierFixedSpaceSmall`. This is the default.
    * `large` - Large space between items. Maps to `NSTouchBarItemIdentifierFixedSpaceLarge`.
    * `flexible` - Take up all available space. Maps to `NSTouchBarItemIdentifierFlexibleSpace`.

### Instance Properties

The following properties are available on instances of `TouchBarSpacer`:

#### `touchBarSpacer.size`

A `string` representing the size of the spacer.  Can be `small`, `large` or `flexible`.

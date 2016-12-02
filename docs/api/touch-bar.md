## Class: TouchBar

> Create TouchBar layouts for native macOS applications

Process: [Main](../tutorial/quick-start.md#main-process)

### `new TouchBar(items)`

* `items` (TouchBarButton | TouchBarColorPicker | TouchBarGroup | TouchBarLabel | TouchBarPopOver | TouchBarSlider)[]

Creates a new touch bar.  Note any changes to the TouchBar instance
will not affect the rendered TouchBar.  To affect the rendered
TouchBar you **must** use either methods on the TouchBar or methods
on the TouchBar* items

### Instance Methods

The `menu` object has the following instance methods:

#### `touchBar.destroy()`

Immediately destroys the TouchBar instance and will reset the rendered
touch bar.

## Examples

The `TouchBar` class is only available in the main process, it is not currently possible to use in the renderer process **even** through the remote module.

### Main process

An example of creating a touch bar in the main process:

```javascript
const {TouchBar, TouchBarButton} = require('electron')

const touchBar = new TouchBar([
  new TouchBarButton({
    label: 'Example Button',
    click: () => console.log('I was clicked')
  })
])

mainWindow.setTouchBar(touchBar)
```

Window = process.atom_binding('window').Window

# Convient accessors.
setupGetterAndSetter = (constructor, name, getter, setter) ->
  constructor.prototype.__defineGetter__ name, ->
    this[getter].apply(this, arguments)
  constructor.prototype.__defineSetter__ name, ->
    this[setter].apply(this, arguments)

setupGetterAndSetter Window, 'fullscreen', 'isFullscreen', 'setFullscreen'
setupGetterAndSetter Window, 'size', 'getSize', 'setSize'
setupGetterAndSetter Window, 'maximumSize', 'getMaximumSize', 'setMaximumSize'
setupGetterAndSetter Window, 'minimumSize', 'getMinimumSize', 'setMinimumSize'
setupGetterAndSetter Window, 'resizable', 'isResizable', 'setResizable'
setupGetterAndSetter Window, 'alwaysOnTop', 'isAlwaysOnTop', 'setAlwaysOnTop'
setupGetterAndSetter Window, 'position', 'getPosition', 'setPosition'
setupGetterAndSetter Window, 'title', 'getTitle', 'setTitle'
setupGetterAndSetter Window, 'kiosk', 'isKiosk', 'setKiosk'

module.exports = Window

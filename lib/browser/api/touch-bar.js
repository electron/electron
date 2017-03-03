const {EventEmitter} = require('events')

let nextItemID = 1

class TouchBar extends EventEmitter {
  // Bind a touch bar to a window
  static _setOnWindow (touchBar, window) {
    if (window._touchBar != null) {
      window._touchBar._removeFromWindow(window)
    }

    if (touchBar == null) {
      window._setTouchBarItems([])
      return
    }

    if (Array.isArray(touchBar)) {
      touchBar = new TouchBar(touchBar)
    }
    touchBar._addToWindow(window)
  }

  constructor (items) {
    super()

    if (!Array.isArray(items)) {
      throw new Error('Must specify items array as first argument')
    }

    this.windowListeners = {}
    this.items = {}
    this.ordereredItems = []

    const registerItem = (item) => {
      this.items[item.id] = item
      item.on('change', () => {
        this.emit('change', item.id, item.type)
      })
      if (item.child instanceof TouchBar) {
        item.child.ordereredItems.forEach(registerItem)
      }
    }
    items.forEach((item) => {
      if (!(item instanceof TouchBarItem)) {
        throw new Error('Each item must be an instance of TouchBarItem')
      }
      this.ordereredItems.push(item)
      registerItem(item)
    })
  }

  _addToWindow (window) {
    const {id} = window

    // Already added to window
    if (this.windowListeners.hasOwnProperty(id)) return

    window._touchBar = this

    const changeListener = (itemID) => {
      window._refreshTouchBarItem(itemID)
    }
    this.on('change', changeListener)

    const interactionListener = (event, itemID, details) => {
      const item = this.items[itemID]
      if (item != null && item.onInteraction != null) {
        item.onInteraction(details)
      }
    }
    window.on('-touch-bar-interaction', interactionListener)

    const removeListeners = () => {
      this.removeListener('change', changeListener)
      window.removeListener('-touch-bar-interaction', interactionListener)
      window.removeListener('closed', removeListeners)
      window._touchBar = null
      delete this.windowListeners[id]
    }
    window.once('closed', removeListeners)
    this.windowListeners[id] = removeListeners

    window._setTouchBarItems(this.ordereredItems)
  }

  _removeFromWindow (window) {
    const removeListeners = this.windowListeners[window.id]
    if (removeListeners != null) removeListeners()
  }
}

class TouchBarItem extends EventEmitter {
  constructor () {
    super()
    this.id = `${nextItemID++}`
  }

  _addLiveProperty (name, initialValue) {
    const privateName = `_${name}`
    this[privateName] = initialValue
    Object.defineProperty(this, name, {
      get: function () {
        return this[privateName]
      },
      set: function (value) {
        this[privateName] = value
        this.emit('change')
      },
      enumerable: true
    })
  }
}

TouchBar.TouchBarButton = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'button'
    const {click, icon, label, backgroundColor} = config
    this._addLiveProperty('label', label)
    this._addLiveProperty('backgroundColor', backgroundColor)
    this._addLiveProperty('icon', icon)
    if (typeof click === 'function') {
      this.onInteraction = () => {
        config.click()
      }
    }
  }
}

TouchBar.TouchBarColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'colorpicker'
    const {availableColors, change, selectedColor} = config
    this._addLiveProperty('availableColors', availableColors)
    this._addLiveProperty('selectedColor', selectedColor)

    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        this._selectedColor = details.color
        change(details.color)
      }
    }
  }
}

TouchBar.TouchBarGroup = class TouchBarGroup extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'group'
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.child)
    }
  }
}

TouchBar.TouchBarLabel = class TouchBarLabel extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'label'
    this._addLiveProperty('label', config.label)
    this._addLiveProperty('textColor', config.textColor)
  }
}

TouchBar.TouchBarPopover = class TouchBarPopover extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'popover'
    this._addLiveProperty('label', config.label)
    this._addLiveProperty('icon', config.icon)
    this.showCloseButton = config.showCloseButton
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.child)
    }
  }
}

TouchBar.TouchBarSlider = class TouchBarSlider extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'slider'
    const {change, label, minValue, maxValue, value} = config
    this._addLiveProperty('label', label)
    this._addLiveProperty('minValue', minValue)
    this._addLiveProperty('maxValue', maxValue)
    this._addLiveProperty('value', value)

    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        this._value = details.value
        change(details.value)
      }
    }
  }
}

TouchBar.TouchBarSpacer = class TouchBarSpacer extends TouchBarItem {
  constructor (config) {
    super()
    if (config == null) config = {}
    this.type = 'spacer'
    this.size = config.size
  }
}

module.exports = TouchBar

class TouchBar {
  constructor (items) {
    this.items = items
    if (!Array.isArray(items)) {
      throw new Error('The items object provided has to be an array')
    }
    items.forEach((item) => {
      if (!item.id) {
        throw new Error('Each item must be an instance of a TouchBarItem')
      }
    })
  }

  toJSON () {
    return this.items.map((item) => item.toJSON ? item.toJSON() : item)
  }
}

let itemIdIncrementor = 1
const itemEventHandlers = {}

TouchBar._event = (itemType, eventArgs) => {
  let args = eventArgs.slice(1)
  if (itemType === 'slider') {
    args = args.map(val => parseInt(val, 10))
  }
  const idParts = eventArgs[0].split('.')
  const itemId = idParts[idParts.length - 1]
  if (itemEventHandlers[itemId]) itemEventHandlers[itemId](...args)
}

class TouchBarItem {
  constructor (config) {
    this.id = itemIdIncrementor++
    const mConfig = Object.assign({}, config || {})
    Object.defineProperty(this, 'config', {
      configurable: false,
      enumerable: false,
      get: () => mConfig
    })
    this.config.id = `${this.config.id || this.id}`
    if (typeof this.config !== 'object' || this.config === null) {
      throw new Error('Provided config must be a non-null object')
    }
  }

  updateConfig (newConfig) {
    if (!this._owner) {
      throw new Error('Cannot call methods on TouchBarItems without assigning to a BrowserWindow')
    }
    const dupConfig = Object.assign({}, newConfig)
    delete dupConfig.id
    Object.assign(this.config, dupConfig)
    this._owner._updateTouchBarItem(this.toJSON())
  }

  toJSON () {
    return this.config
  }
}

TouchBar.Button = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'button'
    const click = config.click
    if (typeof click === 'function') {
      itemEventHandlers[`${this.id}`] = click
    }
  }
}

TouchBar.ColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'colorpicker'
    const change = this.config.change
    if (typeof change === 'function') {
      itemEventHandlers[`${this.id}`] = change
    }
  }
}

TouchBar.Group = class TouchBarGroup extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'group'
  }

  toJSON () {
    const config = this.config
    return Object.assign({}, config, {
      items: config.items && config.items.toJSON ? config.items.toJSON() : []
    })
  }
}

TouchBar.Label = class TouchBarLabel extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'label'
  }
}

TouchBar.PopOver = class TouchBarPopOver extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'popover'
  }

  toJSON () {
    const config = this.config
    return Object.assign({}, config, {
      touchBar: config.touchBar && config.touchBar.toJSON ? config.touchBar.toJSON() : []
    })
  }
}

TouchBar.Slider = class TouchBarSlider extends TouchBarItem {
  constructor (config) {
    super(config)
    this.config.type = 'slider'
    const change = this.config.change
    if (typeof change === 'function') {
      itemEventHandlers[this.id] = change
    }
  }
}

module.exports = TouchBar

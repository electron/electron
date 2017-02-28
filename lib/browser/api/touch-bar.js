const {EventEmitter} = require('events')

let itemIdIncrementor = 1

class TouchBar extends EventEmitter {
  constructor (items) {
    super()

    if (!Array.isArray(items)) {
      throw new Error('The items object provided has to be an array')
    }

    this.items = {}
    this.ordereredItems = []
    const registerItem = (item) => {
      this.items[item.id] = item
      if (item.child instanceof TouchBar) {
        item.child.ordereredItems.forEach(registerItem)
      }
    }
    items.forEach((item) => {
      this.ordereredItems.push(item)
      if (!(item instanceof TouchBarItem)) {
        throw new Error('Each item must be an instance of a TouchBarItem')
      }
      registerItem(item)
    })

    this.on('interaction', (itemID, details) => {
      const item = this.items[itemID]
      if (item != null && item.onInteraction != null) {
        item.onInteraction(details)
      }
    })
  }
}

class TouchBarItem {
  constructor (config) {
    this.id = `${itemIdIncrementor++}`
  }
}

TouchBar.Button = class TouchBarButton extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'button'
    this.label = config.label
    this.backgroundColor = config.backgroundColor
    this.labelColor = config.labelColor
    this.onInteraction = config.click
  }
}

TouchBar.ColorPicker = class TouchBarColorPicker extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'colorpicker'

    const {change} = config
    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        change(details.color)
      }
    }
  }
}

TouchBar.Group = class TouchBarGroup extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'group'
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.items)
    }
  }
}

TouchBar.Label = class TouchBarLabel extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'label'
    this.label = config.label
  }
}

TouchBar.PopOver = class TouchBarPopOver extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'popover'
    this.label = config.label
    this.showCloseButton = config.showCloseButton
    this.child = config.items
    if (!(this.child instanceof TouchBar)) {
      this.child = new TouchBar(this.items)
    }
  }
}

TouchBar.Slider = class TouchBarSlider extends TouchBarItem {
  constructor (config) {
    super(config)
    this.type = 'slider'
    this.minValue = config.minValue
    this.maxValue = config.maxValue
    this.initialValue = config.initialValue

    const {change} = config
    if (typeof change === 'function') {
      this.onInteraction = (details) => {
        change(details.value)
      }
    }
  }
}

module.exports = TouchBar

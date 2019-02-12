const { expect } = require('chai')
const { screen } = require('electron').remote

describe('screen module', () => {
  describe('screen.getCursorScreenPoint()', () => {
    it('returns a point object', () => {
      const point = screen.getCursorScreenPoint()
      expect(point.x).to.be.a('number')
      expect(point.y).to.be.a('number')
    })
  })

  describe('screen.getPrimaryDisplay()', () => {
    it('returns a display object', () => {
      const display = screen.getPrimaryDisplay()
      expect(display).to.be.an('object')
    })

    it('has the correct non-object properties', function () {
      if (process.platform === 'linux') this.skip()
      const display = screen.getPrimaryDisplay()

      expect(display).to.have.a.property('scaleFactor').that.is.a('number')
      expect(display).to.have.a.property('id').that.is.a('number')
      expect(display).to.have.a.property('rotation').that.is.a('number')
      expect(display).to.have.a.property('touchSupport').that.is.a('string')
      expect(display).to.have.a.property('accelerometerSupport').that.is.a('string')
      expect(display).to.have.a.property('internal').that.is.a('boolean')
      expect(display).to.have.a.property('monochrome').that.is.a('boolean')
      expect(display).to.have.a.property('depthPerComponent').that.is.a('number')
      expect(display).to.have.a.property('colorDepth').that.is.a('number')
    })

    it('has a size object property', function () {
      if (process.platform === 'linux') this.skip()
      const display = screen.getPrimaryDisplay()

      expect(display).to.have.a.property('size').that.is.an('object')
      const size = display.size
      expect(size).to.have.a.property('width').that.is.greaterThan(0)
      expect(size).to.have.a.property('height').that.is.greaterThan(0)
    })

    it('has a workAreaSize object property', function () {
      if (process.platform === 'linux') this.skip()
      const display = screen.getPrimaryDisplay()

      expect(display).to.have.a.property('workAreaSize').that.is.an('object')
      const workAreaSize = display.workAreaSize
      expect(workAreaSize).to.have.a.property('width').that.is.greaterThan(0)
      expect(workAreaSize).to.have.a.property('height').that.is.greaterThan(0)
    })

    it('has a bounds object property', function () {
      if (process.platform === 'linux') this.skip()
      const display = screen.getPrimaryDisplay()

      expect(display).to.have.a.property('bounds').that.is.an('object')
      const bounds = display.bounds
      expect(bounds).to.have.a.property('x').that.is.a('number')
      expect(bounds).to.have.a.property('y').that.is.a('number')
      expect(bounds).to.have.a.property('width').that.is.greaterThan(0)
      expect(bounds).to.have.a.property('height').that.is.greaterThan(0)
    })

    it('has a workArea object property', function () {
      const display = screen.getPrimaryDisplay()

      expect(display).to.have.a.property('workArea').that.is.an('object')
      const workArea = display.workArea
      expect(workArea).to.have.a.property('x').that.is.a('number')
      expect(workArea).to.have.a.property('y').that.is.a('number')
      expect(workArea).to.have.a.property('width').that.is.greaterThan(0)
      expect(workArea).to.have.a.property('height').that.is.greaterThan(0)
    })
  })
})

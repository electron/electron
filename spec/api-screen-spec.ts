import { expect } from 'chai';
import { screen } from 'electron/main';

describe('screen module', () => {
  describe('methods reassignment', () => {
    it('works for a selected method', () => {
      const originalFunction = screen.getPrimaryDisplay;
      try {
        (screen as any).getPrimaryDisplay = () => null;
        expect(screen.getPrimaryDisplay()).to.be.null();
      } finally {
        screen.getPrimaryDisplay = originalFunction;
      }
    });
  });

  describe('screen.getCursorScreenPoint()', () => {
    it('returns a point object', () => {
      const point = screen.getCursorScreenPoint();
      expect(point.x).to.be.a('number');
      expect(point.y).to.be.a('number');
    });
  });

  describe('screen.getPrimaryDisplay()', () => {
    it('returns a display object', () => {
      const display = screen.getPrimaryDisplay();
      expect(display).to.be.an('object');
    });

    it('has the correct non-object properties', function () {
      if (process.platform === 'linux') this.skip();
      const display = screen.getPrimaryDisplay();

      expect(display).to.have.property('scaleFactor').that.is.a('number');
      expect(display).to.have.property('id').that.is.a('number');
      expect(display).to.have.property('rotation').that.is.a('number');
      expect(display).to.have.property('touchSupport').that.is.a('string');
      expect(display).to.have.property('accelerometerSupport').that.is.a('string');
      expect(display).to.have.property('internal').that.is.a('boolean');
      expect(display).to.have.property('monochrome').that.is.a('boolean');
      expect(display).to.have.property('depthPerComponent').that.is.a('number');
      expect(display).to.have.property('colorDepth').that.is.a('number');
      expect(display).to.have.property('colorSpace').that.is.a('string');
      expect(display).to.have.property('displayFrequency').that.is.a('number');
    });

    it('has a size object property', function () {
      if (process.platform === 'linux') this.skip();
      const display = screen.getPrimaryDisplay();

      expect(display).to.have.property('size').that.is.an('object');
      const size = display.size;
      expect(size).to.have.property('width').that.is.greaterThan(0);
      expect(size).to.have.property('height').that.is.greaterThan(0);
    });

    it('has a workAreaSize object property', function () {
      if (process.platform === 'linux') this.skip();
      const display = screen.getPrimaryDisplay();

      expect(display).to.have.property('workAreaSize').that.is.an('object');
      const workAreaSize = display.workAreaSize;
      expect(workAreaSize).to.have.property('width').that.is.greaterThan(0);
      expect(workAreaSize).to.have.property('height').that.is.greaterThan(0);
    });

    it('has a bounds object property', function () {
      if (process.platform === 'linux') this.skip();
      const display = screen.getPrimaryDisplay();

      expect(display).to.have.property('bounds').that.is.an('object');
      const bounds = display.bounds;
      expect(bounds).to.have.property('x').that.is.a('number');
      expect(bounds).to.have.property('y').that.is.a('number');
      expect(bounds).to.have.property('width').that.is.greaterThan(0);
      expect(bounds).to.have.property('height').that.is.greaterThan(0);
    });

    it('has a workArea object property', function () {
      const display = screen.getPrimaryDisplay();

      expect(display).to.have.property('workArea').that.is.an('object');
      const workArea = display.workArea;
      expect(workArea).to.have.property('x').that.is.a('number');
      expect(workArea).to.have.property('y').that.is.a('number');
      expect(workArea).to.have.property('width').that.is.greaterThan(0);
      expect(workArea).to.have.property('height').that.is.greaterThan(0);
    });
  });
});

import { Display, screen, desktopCapturer } from 'electron/main';

import { expect } from 'chai';

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

  describe('screen.getAllDisplays', () => {
    it('returns an array of displays', () => {
      const displays = screen.getAllDisplays();
      expect(displays).to.be.an('array').with.lengthOf.at.least(1);
      for (const display of displays) {
        expect(display).to.be.an('object');
      }
    });

    it('returns displays with IDs matching desktopCapturer source display IDs', async () => {
      const displayIds = screen.getAllDisplays().map(d => `${d.id}`);

      const sources = await desktopCapturer.getSources({ types: ['screen'] });
      const sourceIds = sources.map(s => s.display_id);

      expect(displayIds).to.have.length(sources.length);
      expect(displayIds).to.have.same.members(sourceIds);
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
    let display: Display | null = null;

    beforeEach(() => {
      display = screen.getPrimaryDisplay();
    });

    afterEach(() => {
      display = null;
    });

    it('returns a display object', () => {
      expect(display).to.be.an('object');
    });

    it('has the correct non-object properties', function () {
      expect(display).to.have.property('accelerometerSupport').that.is.a('string');
      expect(display).to.have.property('colorDepth').that.is.a('number');
      expect(display).to.have.property('colorSpace').that.is.a('string');
      expect(display).to.have.property('depthPerComponent').that.is.a('number');
      expect(display).to.have.property('detected').that.is.a('boolean');
      expect(display).to.have.property('displayFrequency').that.is.a('number');
      expect(display).to.have.property('id').that.is.a('number');
      expect(display).to.have.property('internal').that.is.a('boolean');
      expect(display).to.have.property('label').that.is.a('string');
      expect(display).to.have.property('monochrome').that.is.a('boolean');
      expect(display).to.have.property('scaleFactor').that.is.a('number');
      expect(display).to.have.property('rotation').that.is.a('number');
      expect(display).to.have.property('touchSupport').that.is.a('string');
    });

    it('has a maximumCursorSize object property', () => {
      expect(display).to.have.property('maximumCursorSize').that.is.an('object');

      const { maximumCursorSize } = display!;
      expect(maximumCursorSize).to.have.property('width').that.is.a('number');
      expect(maximumCursorSize).to.have.property('height').that.is.a('number');
    });

    it('has a nativeOrigin object property', () => {
      expect(display).to.have.property('nativeOrigin').that.is.an('object');

      const { nativeOrigin } = display!;
      expect(nativeOrigin).to.have.property('x').that.is.a('number');
      expect(nativeOrigin).to.have.property('y').that.is.a('number');
    });

    it('has a size object property', () => {
      expect(display).to.have.property('size').that.is.an('object');

      const { size } = display!;

      if (process.platform === 'linux') {
        expect(size).to.have.property('width').that.is.a('number');
        expect(size).to.have.property('height').that.is.a('number');
      } else {
        expect(size).to.have.property('width').that.is.greaterThan(0);
        expect(size).to.have.property('height').that.is.greaterThan(0);
      }
    });

    it('has a workAreaSize object property', () => {
      expect(display).to.have.property('workAreaSize').that.is.an('object');

      const { workAreaSize } = display!;

      if (process.platform === 'linux') {
        expect(workAreaSize).to.have.property('width').that.is.a('number');
        expect(workAreaSize).to.have.property('height').that.is.a('number');
      } else {
        expect(workAreaSize).to.have.property('width').that.is.greaterThan(0);
        expect(workAreaSize).to.have.property('height').that.is.greaterThan(0);
      }
    });

    it('has a bounds object property', () => {
      expect(display).to.have.property('bounds').that.is.an('object');

      const { bounds } = display!;
      expect(bounds).to.have.property('x').that.is.a('number');
      expect(bounds).to.have.property('y').that.is.a('number');

      if (process.platform === 'linux') {
        expect(bounds).to.have.property('width').that.is.a('number');
        expect(bounds).to.have.property('height').that.is.a('number');
      } else {
        expect(bounds).to.have.property('width').that.is.greaterThan(0);
        expect(bounds).to.have.property('height').that.is.greaterThan(0);
      }
    });

    it('has a workArea object property', () => {
      expect(display).to.have.property('workArea').that.is.an('object');

      const { workArea } = display!;
      expect(workArea).to.have.property('x').that.is.a('number');
      expect(workArea).to.have.property('y').that.is.a('number');
      expect(workArea).to.have.property('width').that.is.greaterThan(0);
      expect(workArea).to.have.property('height').that.is.greaterThan(0);
    });
  });
});

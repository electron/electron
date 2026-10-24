import { type Display, screen, desktopCapturer } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

describe('screen module', { tags: ['serial'] }, () => {
  describe('methods reassignment', () => {
    it('works for a selected method', () => {
      const originalFunction = screen.getPrimaryDisplay;
      try {
        (screen as any).getPrimaryDisplay = () => null;
        expect(screen.getPrimaryDisplay()).toBeNull();
      } finally {
        screen.getPrimaryDisplay = originalFunction;
      }
    });
  });

  describe('screen.getAllDisplays', () => {
    it('returns an array of displays', () => {
      const displays = screen.getAllDisplays();
      expect(displays).toBeInstanceOf(Array);
      expect(displays.length).toBeGreaterThanOrEqual(1);
      for (const display of displays) {
        expect(display).toBeTypeOf('object');
      }
    });

    it('returns displays with IDs matching desktopCapturer source display IDs', async () => {
      const displayIds = screen.getAllDisplays().map((d) => `${d.id}`);

      const sources = await desktopCapturer.getSources({ types: ['screen'] });
      const sourceIds = sources.map((s) => s.display_id);

      expect(displayIds).toHaveLength(sources.length);
      expect(displayIds).toHaveLength(sourceIds.length);
      expect(displayIds).toEqual(expect.arrayContaining(sourceIds));
    });
  });

  describe('screen.getCursorScreenPoint()', () => {
    it('returns a point object', () => {
      const point = screen.getCursorScreenPoint();
      expect(point.x).toBeTypeOf('number');
      expect(point.y).toBeTypeOf('number');
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
      expect(display).toBeTypeOf('object');
    });

    it('has the correct non-object properties', () => {
      expect(display).toHaveProperty('accelerometerSupport', expect.any(String));
      expect(display).toHaveProperty('colorDepth', expect.any(Number));
      expect(display).toHaveProperty('colorSpace', expect.any(String));
      expect(display).toHaveProperty('depthPerComponent', expect.any(Number));
      expect(display).toHaveProperty('detected', expect.any(Boolean));
      expect(display).toHaveProperty('displayFrequency', expect.any(Number));
      expect(display).toHaveProperty('id', expect.any(Number));
      expect(display).toHaveProperty('internal', expect.any(Boolean));
      expect(display).toHaveProperty('label', expect.any(String));
      expect(display).toHaveProperty('monochrome', expect.any(Boolean));
      expect(display).toHaveProperty('scaleFactor', expect.any(Number));
      expect(display).toHaveProperty('rotation', expect.any(Number));
      expect(display).toHaveProperty('touchSupport', expect.any(String));
    });

    it('has a maximumCursorSize object property', () => {
      expect(display).toHaveProperty('maximumCursorSize', expect.any(Object));

      const { maximumCursorSize } = display!;
      expect(maximumCursorSize).toHaveProperty('width', expect.any(Number));
      expect(maximumCursorSize).toHaveProperty('height', expect.any(Number));
    });

    it('has a nativeOrigin object property', () => {
      expect(display).toHaveProperty('nativeOrigin', expect.any(Object));

      const { nativeOrigin } = display!;
      expect(nativeOrigin).toHaveProperty('x', expect.any(Number));
      expect(nativeOrigin).toHaveProperty('y', expect.any(Number));
    });

    it('has a size object property', () => {
      expect(display).toHaveProperty('size', expect.any(Object));

      const { size } = display!;

      if (process.platform === 'linux') {
        expect(size).toHaveProperty('width', expect.any(Number));
        expect(size).toHaveProperty('height', expect.any(Number));
      } else {
        expect(size).toHaveProperty('width');
        expect(size.width).toBeGreaterThan(0);
        expect(size).toHaveProperty('height');
        expect(size.height).toBeGreaterThan(0);
      }
    });

    it('has a workAreaSize object property', () => {
      expect(display).toHaveProperty('workAreaSize', expect.any(Object));

      const { workAreaSize } = display!;

      if (process.platform === 'linux') {
        expect(workAreaSize).toHaveProperty('width', expect.any(Number));
        expect(workAreaSize).toHaveProperty('height', expect.any(Number));
      } else {
        expect(workAreaSize).toHaveProperty('width');
        expect(workAreaSize.width).toBeGreaterThan(0);
        expect(workAreaSize).toHaveProperty('height');
        expect(workAreaSize.height).toBeGreaterThan(0);
      }
    });

    it('has a bounds object property', () => {
      expect(display).toHaveProperty('bounds', expect.any(Object));

      const { bounds } = display!;
      expect(bounds).toHaveProperty('x', expect.any(Number));
      expect(bounds).toHaveProperty('y', expect.any(Number));

      if (process.platform === 'linux') {
        expect(bounds).toHaveProperty('width', expect.any(Number));
        expect(bounds).toHaveProperty('height', expect.any(Number));
      } else {
        expect(bounds).toHaveProperty('width');
        expect(bounds.width).toBeGreaterThan(0);
        expect(bounds).toHaveProperty('height');
        expect(bounds.height).toBeGreaterThan(0);
      }
    });

    it('has a workArea object property', () => {
      expect(display).toHaveProperty('workArea', expect.any(Object));

      const { workArea } = display!;
      expect(workArea).toHaveProperty('x', expect.any(Number));
      expect(workArea).toHaveProperty('y', expect.any(Number));
      expect(workArea).toHaveProperty('width');
      expect(workArea.width).toBeGreaterThan(0);
      expect(workArea).toHaveProperty('height');
      expect(workArea.height).toBeGreaterThan(0);
    });
  });
});

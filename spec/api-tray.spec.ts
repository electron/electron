import { nativeImage } from 'electron/common';
import { Menu, Tray } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, ifit } from './lib/spec-helpers.ts';

describe('tray module', { tags: ['serial'] }, () => {
  let tray: Tray;

  beforeEach(() => {
    tray = new Tray(nativeImage.createEmpty());
  });

  afterEach(() => {
    tray.destroy();
    tray = null as any;
  });

  describe('new Tray', () => {
    it('sets the correct class name on the prototype', () => {
      expect(Tray.prototype.constructor.name).toBe('Tray');
    });

    it('throws a descriptive error for a missing file', () => {
      const badPath = path.resolve('I', 'Do', 'Not', 'Exist');
      expect(() => {
        tray = new Tray(badPath);
      }).toThrow(/Failed to load image from path (.+)/);
    });

    ifit(process.platform !== 'linux')('throws a descriptive error if an invalid guid is given', () => {
      expect(() => {
        tray = new Tray(nativeImage.createEmpty(), 'I am not a guid');
      }).toThrow('Invalid GUID format');
    });

    ifit(process.platform !== 'linux')('accepts a valid guid', () => {
      expect(() => {
        tray = new Tray(nativeImage.createEmpty(), '0019A433-3526-48BA-A66C-676742C0FEFB');
      }).not.toThrow();
    });

    it('is an instance of Tray', () => {
      expect(tray).toBeInstanceOf(Tray);
    });
  });

  ifdescribe(process.platform === 'darwin')('tray get/set ignoreDoubleClickEvents', () => {
    it('returns false by default', () => {
      const ignored = tray.getIgnoreDoubleClickEvents();
      expect(ignored, 'ignored').toBe(false);
    });

    it('can be set to true', () => {
      tray.setIgnoreDoubleClickEvents(true);

      const ignored = tray.getIgnoreDoubleClickEvents();
      expect(ignored, 'not ignored').toBe(true);
    });
  });

  describe('tray.setContextMenu(menu)', () => {
    it('accepts both null and Menu as parameters', () => {
      expect(() => {
        tray.setContextMenu(new Menu());
      }).not.toThrow();
      expect(() => {
        tray.setContextMenu(null);
      }).not.toThrow();
    });
  });

  describe('tray.destroy()', () => {
    it('destroys a tray', () => {
      expect(tray.isDestroyed(), 'tray should not be destroyed').toBe(false);
      tray.destroy();

      expect(tray.isDestroyed(), 'tray should be destroyed').toBe(true);
    });
  });

  describe('tray.popUpContextMenu()', () => {
    ifit(process.platform === 'win32')('can be called when menu is showing', async () => {
      tray.setContextMenu(Menu.buildFromTemplate([{ label: 'Test' }]));
      const timeout = setTimeout();
      tray.popUpContextMenu();
      await timeout;
      tray.popUpContextMenu();
    });

    it('can be called with a menu', () => {
      const menu = Menu.buildFromTemplate([{ label: 'Test' }]);
      expect(() => {
        tray.popUpContextMenu(menu);
      }).not.toThrow();
    });

    it('can be called with a position', () => {
      expect(() => {
        tray.popUpContextMenu({ x: 0, y: 0 } as any);
      }).not.toThrow();
    });

    it('can be called with a menu and a position', () => {
      const menu = Menu.buildFromTemplate([{ label: 'Test' }]);
      expect(() => {
        tray.popUpContextMenu(menu, { x: 0, y: 0 });
      }).not.toThrow();
    });

    it('throws an error on invalid arguments', () => {
      expect(() => {
        tray.popUpContextMenu({} as any);
      }).toThrow(/index 0/);
      const menu = Menu.buildFromTemplate([{ label: 'Test' }]);
      expect(() => {
        tray.popUpContextMenu(menu, {} as any);
      }).toThrow(/index 1/);
    });
  });

  describe('tray.closeContextMenu()', () => {
    ifit(process.platform === 'win32')('does not crash when called more than once', async () => {
      tray.setContextMenu(Menu.buildFromTemplate([{ label: 'Test' }]));
      const timeout = setTimeout();
      tray.popUpContextMenu();
      await timeout;
      tray.closeContextMenu();
      tray.closeContextMenu();
    });
  });

  describe('tray.getBounds()', () => {
    afterEach(() => {
      tray.destroy();
    });

    ifit(process.platform !== 'linux')('returns a bounds object', () => {
      const bounds = tray.getBounds();
      expect(bounds).toBeTypeOf('object');
      expect(Object.keys(bounds).sort()).toEqual(['height', 'width', 'x', 'y']);
    });
  });

  describe('tray.setImage(image)', () => {
    it('throws a descriptive error for a missing file', () => {
      const badPath = path.resolve('I', 'Do', 'Not', 'Exist');
      expect(() => {
        tray.setImage(badPath);
      }).toThrow(/Failed to load image from path (.+)/);
    });

    it('accepts empty image', () => {
      tray.setImage(nativeImage.createEmpty());
    });
  });

  describe('tray.setPressedImage(image)', () => {
    it('throws a descriptive error for a missing file', () => {
      const badPath = path.resolve('I', 'Do', 'Not', 'Exist');
      expect(() => {
        tray.setPressedImage(badPath);
      }).toThrow(/Failed to load image from path (.+)/);
    });

    it('accepts empty image', () => {
      tray.setPressedImage(nativeImage.createEmpty());
    });
  });

  ifdescribe(process.platform === 'win32')('tray.displayBalloon(image)', () => {
    it('throws a descriptive error for a missing file', () => {
      const badPath = path.resolve('I', 'Do', 'Not', 'Exist');
      expect(() => {
        tray.displayBalloon({
          title: 'title',
          content: 'wow content',
          icon: badPath
        });
      }).toThrow(/Failed to load image from path (.+)/);
    });

    it('accepts an empty image', () => {
      tray.displayBalloon({
        title: 'title',
        content: 'wow content',
        icon: nativeImage.createEmpty()
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('tray get/set title', () => {
    it('sets/gets non-empty title', () => {
      const title = 'Hello World!';
      tray.setTitle(title);
      const newTitle = tray.getTitle();

      expect(newTitle).toBe(title);
    });

    it('sets/gets empty title', () => {
      const title = '';
      tray.setTitle(title);
      const newTitle = tray.getTitle();

      expect(newTitle).toBe(title);
    });

    it('can have an options object passed in', () => {
      expect(() => {
        tray.setTitle('Hello World!', {});
      }).not.toThrow();
    });

    it('throws when the options parameter is not an object', () => {
      expect(() => {
        tray.setTitle('Hello World!', 'test' as any);
      }).toThrow(/setTitle options must be an object/);
    });

    it('can have a font type option set', () => {
      expect(() => {
        tray.setTitle('Hello World!', { fontType: 'monospaced' });
        tray.setTitle('Hello World!', { fontType: 'monospacedDigit' });
      }).not.toThrow();
    });

    it('throws when the font type is specified but is not a string', () => {
      expect(() => {
        tray.setTitle('Hello World!', { fontType: 5.4 as any });
      }).toThrow(/fontType must be one of 'monospaced' or 'monospacedDigit'/);
    });

    it('throws on invalid font types', () => {
      expect(() => {
        tray.setTitle('Hello World!', { fontType: 'blep' as any });
      }).toThrow(/fontType must be one of 'monospaced' or 'monospacedDigit'/);
    });
  });
});

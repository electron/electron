import { globalShortcut } from 'electron/main';

import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { singleModifierCombinations, doubleModifierCombinations } from './lib/accelerator-helpers.ts';
import { ifdescribe } from './lib/spec-helpers.ts';

ifdescribe(process.platform !== 'win32')('globalShortcut module', { tags: ['serial'] }, () => {
  beforeEach(() => {
    globalShortcut.unregisterAll();
  });

  afterEach(() => {
    globalShortcut.unregisterAll();
  });

  describe('events', () => {
    it('is an event emitter', () => {
      expect(globalShortcut.on).toBeTypeOf('function');
      expect(globalShortcut.removeListener).toBeTypeOf('function');
      const listener = () => {};
      globalShortcut.on('registration-resolved', listener);
      expect(globalShortcut.listenerCount('registration-resolved')).toBe(1);
      globalShortcut.removeListener('registration-resolved', listener);
      expect(globalShortcut.listenerCount('registration-resolved')).toBe(0);
    });
  });

  describe('register', () => {
    it('can register and unregister single accelerators', () => {
      const combinations = [...singleModifierCombinations, ...doubleModifierCombinations];

      combinations.forEach((accelerator) => {
        expect(globalShortcut.isRegistered(accelerator), `Initially registered for ${accelerator}`).toBe(false);

        globalShortcut.register(accelerator, () => {});
        expect(globalShortcut.isRegistered(accelerator), `Registration failed for ${accelerator}`).toBe(true);

        globalShortcut.unregister(accelerator);
        expect(globalShortcut.isRegistered(accelerator), `Unregistration failed for ${accelerator}`).toBe(false);

        globalShortcut.register(accelerator, () => {});
        expect(globalShortcut.isRegistered(accelerator), `Re-registration failed for ${accelerator}`).toBe(true);

        globalShortcut.unregisterAll();
        expect(globalShortcut.isRegistered(accelerator), `Re-unregistration failed for ${accelerator}`).toBe(false);
      });
    });

    it('returns true on successful registration', () => {
      const result = globalShortcut.register('CmdOrCtrl+Q', () => {});
      expect(result).toBe(true);
    });

    it('can re-register the same accelerator without error', () => {
      globalShortcut.register('CmdOrCtrl+Z', () => {});
      expect(() => {
        globalShortcut.register('CmdOrCtrl+Z', () => {});
      }).not.toThrow();
      expect(globalShortcut.isRegistered('CmdOrCtrl+Z')).toBe(true);
    });
  });

  describe('registerAll', () => {
    it('can register and unregister multiple accelerators', () => {
      const accelerators = ['CmdOrCtrl+X', 'CmdOrCtrl+Y'];

      expect(globalShortcut.isRegistered(accelerators[0]), 'first initially unregistered').toBe(false);
      expect(globalShortcut.isRegistered(accelerators[1]), 'second initially unregistered').toBe(false);

      globalShortcut.registerAll(accelerators, () => {});

      expect(globalShortcut.isRegistered(accelerators[0]), 'first registration worked').toBe(true);
      expect(globalShortcut.isRegistered(accelerators[1]), 'second registration worked').toBe(true);

      globalShortcut.unregisterAll();

      expect(globalShortcut.isRegistered(accelerators[0]), 'first unregistered').toBe(false);
      expect(globalShortcut.isRegistered(accelerators[1]), 'second unregistered').toBe(false);
    });

    it('returns true on successful registration', () => {
      const result = globalShortcut.registerAll(['CmdOrCtrl+Q', 'CmdOrCtrl+W'], () => {});
      expect(result).toBe(true);
    });

    it('does not crash when registering media keys as global shortcuts', () => {
      const accelerators = [
        'VolumeUp',
        'VolumeDown',
        'VolumeMute',
        'MediaNextTrack',
        'MediaPreviousTrack',
        'MediaStop',
        'MediaPlayPause'
      ];

      expect(() => {
        globalShortcut.registerAll(accelerators, () => {});
      }).not.toThrow();
    });
  });

  describe('shortcut objects', () => {
    type ShortcutInput = Parameters<typeof globalShortcut.register>[0];

    const registerRaw = (shortcut: unknown) => globalShortcut.register(shortcut as ShortcutInput, () => {});

    it('registers a shortcut given as an object', () => {
      const result = globalShortcut.register(
        { accelerator: 'CmdOrCtrl+L', description: 'Focus location bar' },
        () => {}
      );
      expect(result).toBe(true);
      expect(globalShortcut.isRegistered('CmdOrCtrl+L')).toBe(true);
    });

    it('ignores the description when matching a shortcut', () => {
      globalShortcut.register({ accelerator: 'CmdOrCtrl+M', description: 'First' }, () => {});
      globalShortcut.register({ accelerator: 'CmdOrCtrl+M', description: 'Second' }, () => {});

      expect(globalShortcut.isRegistered('CmdOrCtrl+M'), 'description does not change identity').toBe(true);

      globalShortcut.unregister({ accelerator: 'CmdOrCtrl+M', description: 'Second' });
      expect(globalShortcut.isRegistered('CmdOrCtrl+M')).toBe(false);
    });

    it('accepts a mix of strings and objects in registerAll', () => {
      globalShortcut.registerAll(['CmdOrCtrl+1', { accelerator: 'CmdOrCtrl+2', description: 'Second' }], () => {});

      expect(globalShortcut.isRegistered('CmdOrCtrl+1')).toBe(true);
      expect(globalShortcut.isRegistered('CmdOrCtrl+2')).toBe(true);
    });

    it('accepts either form in isRegistered and unregister', () => {
      globalShortcut.register('CmdOrCtrl+0', () => {});
      expect(globalShortcut.isRegistered({ accelerator: 'CmdOrCtrl+0' })).toBe(true);
      globalShortcut.unregister({ accelerator: 'CmdOrCtrl+0' });
      expect(globalShortcut.isRegistered('CmdOrCtrl+0')).toBe(false);

      globalShortcut.register({ accelerator: 'CmdOrCtrl+9' }, () => {});
      expect(globalShortcut.isRegistered('CmdOrCtrl+9')).toBe(true);
      globalShortcut.unregister('CmdOrCtrl+9');
      expect(globalShortcut.isRegistered('CmdOrCtrl+9')).toBe(false);
    });

    it('throws when the accelerator property is missing', () => {
      expect(() => registerRaw({})).toThrow();
    });

    it('throws when the accelerator property is not a string', () => {
      expect(() => registerRaw({ accelerator: 42 })).toThrow();
    });

    it('throws when the description property is not a string', () => {
      expect(() => registerRaw({ accelerator: 'CmdOrCtrl+9', description: 42 })).toThrow();
    });

    it('throws for an invalid accelerator given as an object', () => {
      expect(() => registerRaw({ accelerator: 'NotAKey' })).toThrow();
    });
  });

  describe('isRegistered', () => {
    it('returns false for an accelerator that was never registered', () => {
      expect(globalShortcut.isRegistered('CmdOrCtrl+Shift+F9')).toBe(false);
    });

    it('returns false after the accelerator is unregistered', () => {
      globalShortcut.register('CmdOrCtrl+J', () => {});
      globalShortcut.unregister('CmdOrCtrl+J');
      expect(globalShortcut.isRegistered('CmdOrCtrl+J')).toBe(false);
    });
  });

  describe('unregister', () => {
    it('does not throw when unregistering a non-registered accelerator', () => {
      expect(() => {
        globalShortcut.unregister('CmdOrCtrl+Shift+F8');
      }).not.toThrow();
    });

    it('does not affect other registered shortcuts', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});
      globalShortcut.register('CmdOrCtrl+B', () => {});
      globalShortcut.register('CmdOrCtrl+C', () => {});

      globalShortcut.unregister('CmdOrCtrl+B');

      expect(globalShortcut.isRegistered('CmdOrCtrl+A'), 'A should still be registered').toBe(true);
      expect(globalShortcut.isRegistered('CmdOrCtrl+B'), 'B should be unregistered').toBe(false);
      expect(globalShortcut.isRegistered('CmdOrCtrl+C'), 'C should still be registered').toBe(true);
    });
  });

  describe('unregisterAll', () => {
    it('does not throw when no shortcuts are registered', () => {
      expect(() => {
        globalShortcut.unregisterAll();
      }).not.toThrow();
    });

    it('unregisters all previously registered shortcuts', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});
      globalShortcut.register('CmdOrCtrl+B', () => {});
      globalShortcut.register('CmdOrCtrl+C', () => {});

      globalShortcut.unregisterAll();

      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).toBe(false);
      expect(globalShortcut.isRegistered('CmdOrCtrl+B')).toBe(false);
      expect(globalShortcut.isRegistered('CmdOrCtrl+C')).toBe(false);
    });

    it('allows re-registration after clearing all shortcuts', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});
      globalShortcut.unregisterAll();

      const result = globalShortcut.register('CmdOrCtrl+A', () => {});
      expect(result).toBe(true);
      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).toBe(true);
    });
  });

  describe('setSuspended / isSuspended', () => {
    afterEach(() => {
      globalShortcut.setSuspended(false);
    });

    it('is not suspended by default', () => {
      expect(globalShortcut.isSuspended()).toBe(false);
    });

    it('can suspend and resume shortcut handling', () => {
      globalShortcut.setSuspended(true);
      expect(globalShortcut.isSuspended()).toBe(true);

      globalShortcut.setSuspended(false);
      expect(globalShortcut.isSuspended()).toBe(false);
    });

    it('can be called multiple times with the same value', () => {
      globalShortcut.setSuspended(true);
      globalShortcut.setSuspended(true);
      expect(globalShortcut.isSuspended()).toBe(true);

      globalShortcut.setSuspended(false);
      globalShortcut.setSuspended(false);
      expect(globalShortcut.isSuspended()).toBe(false);
    });

    it('does not affect existing registrations', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});

      globalShortcut.setSuspended(true);
      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).toBe(true);

      globalShortcut.setSuspended(false);
      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).toBe(true);
    });
  });
});

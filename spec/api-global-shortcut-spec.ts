import { globalShortcut } from 'electron/main';

import { expect } from 'chai';

import { singleModifierCombinations, doubleModifierCombinations } from './lib/accelerator-helpers';
import { ifdescribe } from './lib/spec-helpers';

ifdescribe(process.platform !== 'win32')('globalShortcut module', () => {
  beforeEach(() => {
    globalShortcut.unregisterAll();
  });

  afterEach(() => {
    globalShortcut.unregisterAll();
  });

  describe('register', () => {
    it('can register and unregister single accelerators', () => {
      const combinations = [...singleModifierCombinations, ...doubleModifierCombinations];

      combinations.forEach((accelerator) => {
        expect(globalShortcut.isRegistered(accelerator)).to.be.false(`Initially registered for ${accelerator}`);

        globalShortcut.register(accelerator, () => {});
        expect(globalShortcut.isRegistered(accelerator)).to.be.true(`Registration failed for ${accelerator}`);

        globalShortcut.unregister(accelerator);
        expect(globalShortcut.isRegistered(accelerator)).to.be.false(`Unregistration failed for ${accelerator}`);

        globalShortcut.register(accelerator, () => {});
        expect(globalShortcut.isRegistered(accelerator)).to.be.true(`Re-registration failed for ${accelerator}`);

        globalShortcut.unregisterAll();
        expect(globalShortcut.isRegistered(accelerator)).to.be.false(`Re-unregistration failed for ${accelerator}`);
      });
    });

    it('returns true on successful registration', () => {
      const result = globalShortcut.register('CmdOrCtrl+Q', () => {});
      expect(result).to.be.true();
    });

    it('can re-register the same accelerator without error', () => {
      globalShortcut.register('CmdOrCtrl+Z', () => {});
      expect(() => {
        globalShortcut.register('CmdOrCtrl+Z', () => {});
      }).to.not.throw();
      expect(globalShortcut.isRegistered('CmdOrCtrl+Z')).to.be.true();
    });
  });

  describe('registerAll', () => {
    it('can register and unregister multiple accelerators', () => {
      const accelerators = ['CmdOrCtrl+X', 'CmdOrCtrl+Y'];

      expect(globalShortcut.isRegistered(accelerators[0])).to.be.false('first initially unregistered');
      expect(globalShortcut.isRegistered(accelerators[1])).to.be.false('second initially unregistered');

      globalShortcut.registerAll(accelerators, () => {});

      expect(globalShortcut.isRegistered(accelerators[0])).to.be.true('first registration worked');
      expect(globalShortcut.isRegistered(accelerators[1])).to.be.true('second registration worked');

      globalShortcut.unregisterAll();

      expect(globalShortcut.isRegistered(accelerators[0])).to.be.false('first unregistered');
      expect(globalShortcut.isRegistered(accelerators[1])).to.be.false('second unregistered');
    });

    it('returns true on successful registration', () => {
      const result = globalShortcut.registerAll(['CmdOrCtrl+Q', 'CmdOrCtrl+W'], () => {});
      expect(result).to.be.true();
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
      }).to.not.throw();
    });
  });

  describe('isRegistered', () => {
    it('returns false for an accelerator that was never registered', () => {
      expect(globalShortcut.isRegistered('CmdOrCtrl+Shift+F9')).to.be.false();
    });

    it('returns false after the accelerator is unregistered', () => {
      globalShortcut.register('CmdOrCtrl+J', () => {});
      globalShortcut.unregister('CmdOrCtrl+J');
      expect(globalShortcut.isRegistered('CmdOrCtrl+J')).to.be.false();
    });
  });

  describe('unregister', () => {
    it('does not throw when unregistering a non-registered accelerator', () => {
      expect(() => {
        globalShortcut.unregister('CmdOrCtrl+Shift+F8');
      }).to.not.throw();
    });

    it('does not affect other registered shortcuts', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});
      globalShortcut.register('CmdOrCtrl+B', () => {});
      globalShortcut.register('CmdOrCtrl+C', () => {});

      globalShortcut.unregister('CmdOrCtrl+B');

      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).to.be.true('A should still be registered');
      expect(globalShortcut.isRegistered('CmdOrCtrl+B')).to.be.false('B should be unregistered');
      expect(globalShortcut.isRegistered('CmdOrCtrl+C')).to.be.true('C should still be registered');
    });
  });

  describe('unregisterAll', () => {
    it('does not throw when no shortcuts are registered', () => {
      expect(() => {
        globalShortcut.unregisterAll();
      }).to.not.throw();
    });

    it('unregisters all previously registered shortcuts', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});
      globalShortcut.register('CmdOrCtrl+B', () => {});
      globalShortcut.register('CmdOrCtrl+C', () => {});

      globalShortcut.unregisterAll();

      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).to.be.false();
      expect(globalShortcut.isRegistered('CmdOrCtrl+B')).to.be.false();
      expect(globalShortcut.isRegistered('CmdOrCtrl+C')).to.be.false();
    });

    it('allows re-registration after clearing all shortcuts', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});
      globalShortcut.unregisterAll();

      const result = globalShortcut.register('CmdOrCtrl+A', () => {});
      expect(result).to.be.true();
      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).to.be.true();
    });
  });

  describe('setSuspended / isSuspended', () => {
    afterEach(() => {
      globalShortcut.setSuspended(false);
    });

    it('is not suspended by default', () => {
      expect(globalShortcut.isSuspended()).to.be.false();
    });

    it('can suspend and resume shortcut handling', () => {
      globalShortcut.setSuspended(true);
      expect(globalShortcut.isSuspended()).to.be.true();

      globalShortcut.setSuspended(false);
      expect(globalShortcut.isSuspended()).to.be.false();
    });

    it('can be called multiple times with the same value', () => {
      globalShortcut.setSuspended(true);
      globalShortcut.setSuspended(true);
      expect(globalShortcut.isSuspended()).to.be.true();

      globalShortcut.setSuspended(false);
      globalShortcut.setSuspended(false);
      expect(globalShortcut.isSuspended()).to.be.false();
    });

    it('does not affect existing registrations', () => {
      globalShortcut.register('CmdOrCtrl+A', () => {});

      globalShortcut.setSuspended(true);
      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).to.be.true();

      globalShortcut.setSuspended(false);
      expect(globalShortcut.isRegistered('CmdOrCtrl+A')).to.be.true();
    });
  });
});

import { globalShortcut } from 'electron/main';

import { expect } from 'chai';

import { ifdescribe } from './lib/spec-helpers';

const modifiers = [
  'CmdOrCtrl',
  'Alt',
  process.platform === 'darwin' ? 'Option' : null,
  'AltGr',
  'Shift',
  'Super',
  'Meta'
].filter(Boolean);

const keyCodes = [
  ...Array.from({ length: 10 }, (_, i) => `${i}`), // 0 to 9
  ...Array.from({ length: 26 }, (_, i) => String.fromCharCode(65 + i)), // A to Z
  ...Array.from({ length: 24 }, (_, i) => `F${i + 1}`), // F1 to F24
  ')', '!', '@', '#', '$', '%', '^', '&', '*', '(', ':', ';', ':', '+', '=',
  '<', ',', '_', '-', '>', '.', '?', '/', '~', '`', '{', ']', '[', '|', '\\',
  '}', '"', 'Plus', 'Space', 'Tab', 'Capslock', 'Numlock', 'Scrolllock',
  'Backspace', 'Delete', 'Insert', 'Return', 'Enter', 'Up', 'Down', 'Left',
  'Right', 'Home', 'End', 'PageUp', 'PageDown', 'Escape', 'Esc', 'PrintScreen',
  'num0', 'num1', 'num2', 'num3', 'num4', 'num5', 'num6', 'num7', 'num8', 'num9',
  'numdec', 'numadd', 'numsub', 'nummult', 'numdiv'
];

ifdescribe(process.platform !== 'win32')('globalShortcut module', () => {
  beforeEach(() => {
    globalShortcut.unregisterAll();
  });

  it('can register and unregister single accelerators', () => {
    const singleModifierCombinations = modifiers.flatMap(
      mod => keyCodes.map(key => {
        return key === '+' ? `${mod}+Plus` : `${mod}+${key}`;
      })
    );

    const doubleModifierCombinations = modifiers.flatMap(
      (mod1, i) => modifiers.slice(i + 1).flatMap(
        mod2 => keyCodes.map(key => {
          return key === '+' ? `${mod1}+${mod2}+Plus` : `${mod1}+${mod2}+${key}`;
        })
      )
    );

    const combinations = [...singleModifierCombinations, ...doubleModifierCombinations];

    combinations.forEach((accelerator) => {
      expect(globalShortcut.isRegistered(accelerator)).to.be.false(`Initially registered for ${accelerator}`);

      globalShortcut.register(accelerator, () => { });
      expect(globalShortcut.isRegistered(accelerator)).to.be.true(`Registration failed for ${accelerator}`);

      globalShortcut.unregister(accelerator);
      expect(globalShortcut.isRegistered(accelerator)).to.be.false(`Unregistration failed for ${accelerator}`);

      globalShortcut.register(accelerator, () => { });
      expect(globalShortcut.isRegistered(accelerator)).to.be.true(`Re-registration failed for ${accelerator}`);

      globalShortcut.unregisterAll();
      expect(globalShortcut.isRegistered(accelerator)).to.be.false(`Re-unregistration failed for ${accelerator}`);
    });
  });

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

  it('does not crash when registering media keys as global shortcuts', () => {
    const accelerators = [
      'VolumeUp',
      'VolumeDown',
      'VolumeMute',
      'MediaNextTrack',
      'MediaPreviousTrack',
      'MediaStop', 'MediaPlayPause'
    ];

    expect(() => {
      globalShortcut.registerAll(accelerators, () => {});
    }).to.not.throw();

    globalShortcut.unregisterAll();
  });
});

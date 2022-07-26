import { expect } from 'chai';
import { globalShortcut } from 'electron/main';
import { ifdescribe } from './spec-helpers';

ifdescribe(process.platform !== 'win32')('globalShortcut module', () => {
  beforeEach(() => {
    globalShortcut.unregisterAll();
  });

  it('can register and unregister single accelerators', () => {
    const accelerator = 'CmdOrCtrl+A+B+C';

    expect(globalShortcut.isRegistered(accelerator)).to.be.false('initially registered');
    globalShortcut.register(accelerator, () => {});
    expect(globalShortcut.isRegistered(accelerator)).to.be.true('registration worked');
    globalShortcut.unregister(accelerator);
    expect(globalShortcut.isRegistered(accelerator)).to.be.false('unregistration worked');

    globalShortcut.register(accelerator, () => {});
    expect(globalShortcut.isRegistered(accelerator)).to.be.true('reregistration worked');
    globalShortcut.unregisterAll();
    expect(globalShortcut.isRegistered(accelerator)).to.be.false('re-unregistration worked');
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

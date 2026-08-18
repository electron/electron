import { globalShortcut } from 'electron/main';

import { expect } from 'chai';
import * as dbus from 'dbus-native';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { promisify } from 'node:util';

import { singleModifierCombinations, doubleModifierCombinations } from './lib/accelerator-helpers';
import { ifdescribe, ifit } from './lib/spec-helpers';

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

  describe('listShortcuts', () => {
    it('returns a promise that settles', async () => {
      try {
        const shortcuts = await globalShortcut.listShortcuts();
        // Resolves only when the GlobalShortcuts desktop portal handles
        // shortcut registration.
        expect(shortcuts).to.be.an('array');
      } catch (error) {
        expect(error).to.be.an.instanceOf(Error);
      }
    });

    ifit(process.platform === 'darwin')('rejects when registration is not handled by the desktop portal', async () => {
      await expect(globalShortcut.listShortcuts()).to.eventually.be.rejectedWith(/GlobalShortcuts desktop portal/);
    });
  });

  // Drives a child Electron on the headless Ozone platform (which has no native
  // shortcut listener, so registration goes through the GlobalShortcuts portal)
  // against the mock portal that script/dbus_mock.py hosts on the fake session
  // bus. The mock keeps approved shortcuts across sessions like a compositor
  // does across application launches.
  ifdescribe(
    process.platform === 'linux' &&
      process.arch !== 'ia32' &&
      !process.arch.startsWith('arm') &&
      !!process.env.DBUS_SESSION_BUS_ADDRESS
  )('with the GlobalShortcuts desktop portal', () => {
    const fixture = path.resolve(__dirname, 'fixtures', 'api', 'global-shortcut-portal');
    const portalPath = '/org/freedesktop/portal/desktop';
    let bus: any;
    let getCalls: () => Promise<any[]>;
    let clearCalls: () => Promise<void>;
    let resetPortal: () => Promise<void>;
    let setApprovedShortcuts: (shortcuts: [string, string, string][]) => Promise<void>;
    let failNextListShortcuts: (count: number) => Promise<void>;
    let userDataDir: string;

    before(async () => {
      bus = dbus.sessionBus();
      const service = bus.getService('org.freedesktop.portal.Desktop');
      const getInterface = promisify(service.getInterface.bind(service));
      const mock: any = await getInterface(portalPath, 'org.freedesktop.DBus.Mock');
      const control: any = await getInterface(portalPath, 'org.electron.spec.GlobalShortcutsMock');
      getCalls = promisify(mock.GetCalls.bind(mock));
      clearCalls = promisify(mock.ClearCalls.bind(mock));
      resetPortal = promisify(control.Reset.bind(control));
      setApprovedShortcuts = promisify(control.SetShortcuts.bind(control));
      failNextListShortcuts = promisify(control.FailNextListShortcuts.bind(control));
      // A stable profile, so shortcut ids stay the same across launches.
      userDataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-global-shortcut-portal-'));
    });

    after(() => {
      bus?.connection.end();
      if (userDataDir) fs.rmSync(userDataDir, { recursive: true, force: true });
    });

    beforeEach(async () => {
      await resetPortal();
      await clearCalls();
    });

    const launch = async (scenario: string, extraArgs: string[] = []) => {
      const child = childProcess.spawn(
        process.execPath,
        [
          fixture,
          `--scenario=${scenario}`,
          '--ozone-platform=headless',
          '--enable-features=GlobalShortcutsPortal',
          `--user-data-dir=${userDataDir}`,
          ...extraArgs
        ],
        {
          // Anything but GNOME, where Chromium keeps the portal listener off.
          env: { ...process.env, XDG_CURRENT_DESKTOP: 'KDE' },
          stdio: ['ignore', 'pipe', 'inherit']
        }
      );
      let stdout = '';
      child.stdout.on('data', (chunk) => {
        stdout += chunk;
      });
      await once(child, 'exit');
      const line = stdout
        .trim()
        .split('\n')
        .filter((l) => l.startsWith('{'))
        .pop();
      expect(line, `fixture output: ${stdout}`).to.be.a('string');
      return JSON.parse(line!);
    };

    // A logged call is [timestamp, method, args]; each arg is a [signature, [value]] pair.
    const portalCalls = async (method: string) => (await getCalls()).filter((call) => call[1] === method);
    const dictValue = (entries: any[], key: string) => entries.find((entry) => entry[0] === key)?.[1][1][0];
    const createdSessionTokens = async () =>
      (await portalCalls('CreateSession')).map((call) => dictValue(call[2][0][1][0], 'session_handle_token') as string);
    const boundShortcutIds = async () =>
      (await portalCalls('BindShortcuts')).flatMap((call) =>
        call[2][1][1][0].map((shortcut: any) => shortcut[0] as string)
      );

    it('lists the shortcuts the portal has approved for the application', async () => {
      await setApprovedShortcuts([
        ['0123456789ABCDEF0123456789ABCDEF-Alt+Shift+K', 'Electron shortcut', 'ALT+SHIFT+K'],
        ['some-other-binding', 'Foreign', 'META+F1']
      ]);
      const { first, concurrent } = await launch('list');
      expect(first).to.deep.equal({
        shortcuts: [
          {
            id: '0123456789ABCDEF0123456789ABCDEF-Alt+Shift+K',
            accelerator: 'Alt+Shift+K',
            description: 'Electron shortcut',
            triggerDescription: 'ALT+SHIFT+K'
          },
          { id: 'some-other-binding', description: 'Foreign', triggerDescription: 'META+F1' }
        ]
      });
      // Concurrent callers share one query, and all queries share one portal
      // session that is never closed.
      expect(concurrent).to.deep.equal([first, first, first]);
      expect(await portalCalls('ListShortcuts')).to.have.lengthOf(2);
      const listerTokens = (await createdSessionTokens()).filter((token) =>
        token.startsWith('electron_list_shortcuts_')
      );
      expect(listerTokens).to.have.lengthOf(1);
      const closedSessions = (await getCalls()).filter((call) => call[1] === 'Close');
      expect(closedSessions).to.have.lengthOf(0);
    });

    it('includes shortcuts registered in the current session', async () => {
      const { registered, after } = await launch('register', ['--register=Alt+Shift+P']);
      expect(registered).to.deep.equal({ 'Alt+Shift+P': true });
      const entry = after.shortcuts.find((shortcut: any) => shortcut.accelerator === 'Alt+Shift+P');
      expect(entry, JSON.stringify(after)).to.exist();
      expect(entry.id).to.match(/^[0-9A-F]{32}-Alt\+Shift\+P$/);
      expect(await boundShortcutIds()).to.include(entry.id);
    });

    it('binds a shortcut registered after the portal reported every command as approved', async () => {
      // The first launch gets Alt+Shift+P approved. On the second launch the
      // portal already lists it, so registering it again needs no bind pass;
      // a shortcut registered afterwards must still reach the portal.
      await launch('register', ['--register=Alt+Shift+P']);
      await clearCalls();
      const { registered, after } = await launch('register', ['--register=Alt+Shift+P', '--register=Ctrl+Shift+Y']);
      expect(registered).to.deep.equal({ 'Alt+Shift+P': true, 'Ctrl+Shift+Y': true });
      const bound = await boundShortcutIds();
      expect(
        bound.some((id) => id.endsWith('-Ctrl+Shift+Y')),
        `bound: ${JSON.stringify(bound)}`
      ).to.be.true();
      expect(after.shortcuts.map((shortcut: any) => shortcut.accelerator)).to.include.members([
        'Alt+Shift+P',
        'Ctrl+Shift+Y'
      ]);
    });

    it('retries a failed query once with a fresh session and rejects after that', async () => {
      await failNextListShortcuts(1);
      let result = await launch('list');
      expect(result.first).to.deep.equal({ shortcuts: [] });
      const listerTokens = (await createdSessionTokens()).filter((token) =>
        token.startsWith('electron_list_shortcuts_')
      );
      expect(listerTokens).to.have.lengthOf(2);

      await failNextListShortcuts(2);
      result = await launch('list');
      expect(result.first.error).to.match(/Failed to list shortcuts from the GlobalShortcuts portal/);
      // Later queries recover.
      expect(result.concurrent[0]).to.deep.equal({ shortcuts: [] });
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

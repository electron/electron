// For these tests we use a fake DBus daemon to verify libnotify interaction
// with the session bus. This requires python-dbusmock to be installed and
// running at $DBUS_SESSION_BUS_ADDRESS.
//
// script/spec-runner.js spawns dbusmock, which sets DBUS_SESSION_BUS_ADDRESS.
//
// See https://pypi.python.org/pypi/python-dbusmock to read about dbusmock.

import { nativeImage } from 'electron/common';
import { app } from 'electron/main';

import { expect } from 'chai';
import * as dbus from 'dbus-native';

import * as path from 'node:path';
import { promisify } from 'node:util';

import { ifdescribe } from './lib/spec-helpers';

const fixturesPath = path.join(__dirname, 'fixtures');

const skip = process.platform !== 'linux' ||
             process.arch === 'ia32' ||
             process.arch.indexOf('arm') === 0 ||
             !process.env.DBUS_SESSION_BUS_ADDRESS;

ifdescribe(!skip)('Notification module (dbus)', () => {
  let mock: any, Notification, getCalls: any, reset: any;
  const realAppName = app.name;
  const realAppVersion = app.getVersion();
  const appName = 'api-notification-dbus-spec';
  const serviceName = 'org.freedesktop.Notifications';

  before(async () => {
    // init app
    app.name = appName;
    app.setDesktopName(`${appName}.desktop`);

    // init DBus
    const path = '/org/freedesktop/Notifications';
    const iface = 'org.freedesktop.DBus.Mock';
    console.log(`session bus: ${process.env.DBUS_SESSION_BUS_ADDRESS}`);
    const bus = dbus.sessionBus();
    const service = bus.getService(serviceName);
    const getInterface = promisify(service.getInterface.bind(service));
    mock = await getInterface(path, iface);
    getCalls = promisify(mock.GetCalls.bind(mock));
    reset = promisify(mock.Reset.bind(mock));
  });

  after(async () => {
    // cleanup dbus
    if (reset) await reset();
    // cleanup app
    app.setName(realAppName);
    app.setVersion(realAppVersion);
  });

  describe(`Notification module using ${serviceName}`, () => {
    function onMethodCalled (done: () => void) {
      function cb (name: string) {
        console.log(`onMethodCalled: ${name}`);
        if (name === 'Notify') {
          mock.removeListener('MethodCalled', cb);
          console.log('done');
          done();
        }
      }
      return cb;
    }

    function unmarshalDBusNotifyHints (dbusHints: any) {
      const o: Record<string, any> = {};
      for (const hint of dbusHints) {
        const key = hint[0];
        const value = hint[1][1][0];
        o[key] = value;
      }
      return o;
    }

    function unmarshalDBusNotifyArgs (dbusArgs: any) {
      return {
        app_name: dbusArgs[0][1][0],
        replaces_id: dbusArgs[1][1][0],
        app_icon: dbusArgs[2][1][0],
        title: dbusArgs[3][1][0],
        body: dbusArgs[4][1][0],
        actions: dbusArgs[5][1][0],
        hints: unmarshalDBusNotifyHints(dbusArgs[6][1][0])
      };
    }

    before(done => {
      mock.on('MethodCalled', onMethodCalled(done));
      // lazy load Notification after we listen to MethodCalled mock signal
      Notification = require('electron').Notification;
      const n = new Notification({
        title: 'title',
        subtitle: 'subtitle',
        body: 'body',
        icon: nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'notification_icon.png')),
        replyPlaceholder: 'replyPlaceholder',
        sound: 'sound',
        closeButtonText: 'closeButtonText'
      });
      n.show();
    });

    it(`should call ${serviceName} to show notifications`, async () => {
      const calls = await getCalls();
      expect(calls).to.be.an('array').of.lengthOf.at.least(1);

      const lastCall = calls[calls.length - 1];
      const methodName = lastCall[1];
      expect(methodName).to.equal('Notify');

      const args = unmarshalDBusNotifyArgs(lastCall[2]);
      expect(args).to.deep.equal({
        app_name: appName,
        replaces_id: 0,
        app_icon: '',
        title: 'title',
        body: 'body',
        actions: [],
        hints: {
          append: 'true',
          image_data: [3, 3, 12, true, 8, 4, Buffer.from([255, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 76, 255, 0, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 38, 255, 255, 0, 0, 0, 255, 0, 0, 0, 0])],
          'desktop-entry': appName,
          'sender-pid': process.pid,
          urgency: 1
        }
      });
    });
  });
});

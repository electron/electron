import { expect } from 'chai';
import * as express from 'express';

import * as http from 'node:http';
import { AddressInfo } from 'node:net';

import {
  getElectronExecutable,
  getMainJsFixturePath,
  getMsixFixturePath,
  getMsixPackageVersion,
  installMsixCertificate,
  installMsixPackage,
  registerExecutableWithIdentity,
  shouldRunMsixTests,
  spawn,
  uninstallMsixPackage,
  unregisterExecutableWithIdentity
} from './lib/msix-helpers';
import { ifdescribe } from './lib/spec-helpers';

const ELECTRON_MSIX_ALIAS = 'ElectronMSIX.exe';
const MAIN_JS_PATH = getMainJsFixturePath();
const MSIX_V1 = getMsixFixturePath('v1');
const MSIX_V2 = getMsixFixturePath('v2');

// We can only test the MSIX updater on Windows
ifdescribe(shouldRunMsixTests)('autoUpdater MSIX behavior', function () {
  this.timeout(120000);

  before(async function () {
    await installMsixCertificate();

    const electronExec = getElectronExecutable();
    await registerExecutableWithIdentity(electronExec);
  });

  after(async function () {
    await unregisterExecutableWithIdentity();
  });

  const launchApp = (executablePath: string, args: string[] = []) => {
    return spawn(executablePath, args);
  };

  const logOnError = (what: any, fn: () => void) => {
    try {
      fn();
    } catch (err) {
      console.error(what);
      throw err;
    }
  };

  it('should launch Electron via MSIX alias', async () => {
    const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, ['--version']);
    logOnError(launchResult, () => {
      expect(launchResult.code).to.equal(0);
    });
  });

  it('should print package identity information', async () => {
    const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [MAIN_JS_PATH, '--printPackageId']);
    logOnError(launchResult, () => {
      expect(launchResult.code).to.equal(0);
      expect(launchResult.out).to.include('Family Name: Electron.Dev.MSIX_rdjwn13tdj8dy');
      expect(launchResult.out).to.include('Package ID: Electron.Dev.MSIX_1.0.0.0_x64__rdjwn13tdj8dy');
      expect(launchResult.out).to.include('Version: 1.0.0.0');
    });
  });

  describe('with update server', () => {
    let port = 0;
    let server: express.Application = null as any;
    let httpServer: http.Server = null as any;
    let requests: express.Request[] = [];

    beforeEach((done) => {
      requests = [];
      server = express();
      server.use((req, res, next) => {
        requests.push(req);
        next();
      });
      httpServer = server.listen(0, '127.0.0.1', () => {
        port = (httpServer.address() as AddressInfo).port;
        done();
      });
    });

    afterEach(async () => {
      if (httpServer) {
        await new Promise<void>(resolve => {
          httpServer.close(() => {
            httpServer = null as any;
            server = null as any;
            resolve();
          });
        });
      }
      await uninstallMsixPackage('com.electron.myapp');
    });

    it('should not update when no update is available', async () => {
      server.get('/update-check', (req, res) => {
        res.status(204).send();
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [MAIN_JS_PATH, '--checkUpdate', `http://localhost:${port}/update-check`]);
      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update not available');
      });
    });

    it('should hit the update endpoint with custom headers when checkForUpdates is called', async () => {
      server.get('/update-check', (req, res) => {
        expect(req.headers['x-appversion']).to.equal('1.0.0');
        expect(req.headers.authorization).to.equal('Bearer test-token');
        res.status(204).send();
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [
        MAIN_JS_PATH,
        '--checkUpdate',
        `http://localhost:${port}/update-check`,
        '--useCustomHeaders'
      ]);

      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update not available');
      });
    });

    it('should update successfully with direct link to MSIX file', async () => {
      await installMsixPackage(MSIX_V1);
      const initialVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(initialVersion).to.equal('1.0.0.0');

      server.get('/update.msix', (req, res) => {
        res.download(MSIX_V2);
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [
        MAIN_JS_PATH,
        '--checkUpdate',
        `http://localhost:${port}/update.msix`
      ]);

      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update available');
        expect(launchResult.out).to.include('Update downloaded');
        expect(launchResult.out).to.include('Release Name: N/A');
        expect(launchResult.out).to.include('Release Notes: N/A');
        expect(launchResult.out).to.include(`Update URL: http://localhost:${port}/update.msix`);
      });

      const updatedVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(updatedVersion).to.equal('2.0.0.0');
    });

    it('should update successfully with JSON response', async () => {
      await installMsixPackage(MSIX_V1);
      const initialVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(initialVersion).to.equal('1.0.0.0');

      const fixedPubDate = '2011-11-11T11:11:11.000Z';
      const expectedDateStr = new Date(fixedPubDate).toDateString();

      server.get('/update-check', (req, res) => {
        res.json({
          url: `http://localhost:${port}/update.msix`,
          name: '2.0.0',
          notes: 'Test release notes',
          pub_date: fixedPubDate
        });
      });

      server.get('/update.msix', (req, res) => {
        res.download(MSIX_V2);
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [MAIN_JS_PATH, '--checkUpdate', `http://localhost:${port}/update-check`]);
      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update available');
        expect(launchResult.out).to.include('Update downloaded');
        expect(launchResult.out).to.include('Release Name: 2.0.0');
        expect(launchResult.out).to.include('Release Notes: Test release notes');
        expect(launchResult.out).to.include(`Release Date: ${expectedDateStr}`);
        expect(launchResult.out).to.include(`Update URL: http://localhost:${port}/update.msix`);
      });

      const updatedVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(updatedVersion).to.equal('2.0.0.0');
    });

    it('should update successfully with static JSON releases file', async () => {
      await installMsixPackage(MSIX_V1);
      const initialVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(initialVersion).to.equal('1.0.0.0');

      const fixedPubDate = '2011-11-11T11:11:11.000Z';
      const expectedDateStr = new Date(fixedPubDate).toDateString();

      server.get('/update-check', (req, res) => {
        res.json({
          currentRelease: '2.0.0',
          releases: [
            {
              version: '1.0.0',
              updateTo: {
                version: '1.0.0',
                url: `http://localhost:${port}/update-v1.msix`,
                name: '1.0.0',
                notes: 'Initial release',
                pub_date: '2010-10-10T10:10:10.000Z'
              }
            },
            {
              version: '2.0.0',
              updateTo: {
                version: '2.0.0',
                url: `http://localhost:${port}/update-v2.msix`,
                name: '2.0.0',
                notes: 'Test release notes for static format',
                pub_date: fixedPubDate
              }
            }
          ]
        });
      });

      server.get('/update-v2.msix', (req, res) => {
        res.download(MSIX_V2);
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [MAIN_JS_PATH, '--checkUpdate', `http://localhost:${port}/update-check`]);

      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update available');
        expect(launchResult.out).to.include('Update downloaded');
        expect(launchResult.out).to.include('Release Name: 2.0.0');
        expect(launchResult.out).to.include('Release Notes: Test release notes for static format');
        expect(launchResult.out).to.include(`Release Date: ${expectedDateStr}`);
        expect(launchResult.out).to.include(`Update URL: http://localhost:${port}/update-v2.msix`);
      });

      const updatedVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(updatedVersion).to.equal('2.0.0.0');
    });

    it('should not update with update File JSON Format if currentRelease is older than installed version', async () => {
      await installMsixPackage(MSIX_V2);

      server.get('/update-check', (req, res) => {
        res.json({
          currentRelease: '1.0.0',
          releases: [
            {
              version: '1.0.0',
              updateTo: {
                version: '1.0.0',
                url: `http://localhost:${port}/update-v1.msix`,
                name: '1.0.0',
                notes: 'Initial release',
                pub_date: '2010-10-10T10:10:10.000Z'
              }
            }
          ]
        });
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [MAIN_JS_PATH, '--checkUpdate', `http://localhost:${port}/update-check`]);

      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update not available');
      });
    });

    it('should downgrade to older version with JSON server format and allowAnyVersion is true', async () => {
      await installMsixPackage(MSIX_V2);
      const initialVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(initialVersion).to.equal('2.0.0.0');

      const fixedPubDate = '2010-10-10T10:10:10.000Z';
      const expectedDateStr = new Date(fixedPubDate).toDateString();

      server.get('/update-check', (req, res) => {
        res.json({
          url: `http://localhost:${port}/update-v1.msix`,
          name: '1.0.0',
          notes: 'Initial release',
          pub_date: fixedPubDate
        });
      });

      server.get('/update-v1.msix', (req, res) => {
        res.download(MSIX_V1);
      });

      const launchResult = await launchApp(ELECTRON_MSIX_ALIAS, [MAIN_JS_PATH, '--checkUpdate', `http://localhost:${port}/update-check`, '--allowAnyVersion']);

      logOnError(launchResult, () => {
        expect(launchResult.code).to.equal(0);
        expect(requests.length).to.be.greaterThan(0);
        expect(launchResult.out).to.include('Checking for update...');
        expect(launchResult.out).to.include('Update available');
        expect(launchResult.out).to.include('Update downloaded');
        expect(launchResult.out).to.include('Release Name: 1.0.0');
        expect(launchResult.out).to.include('Release Notes: Initial release');
        expect(launchResult.out).to.include(`Release Date: ${expectedDateStr}`);
        expect(launchResult.out).to.include(`Update URL: http://localhost:${port}/update-v1.msix`);
      });

      const downgradedVersion = await getMsixPackageVersion('com.electron.myapp');
      expect(downgradedVersion).to.equal('1.0.0.0');
    });
  });
});

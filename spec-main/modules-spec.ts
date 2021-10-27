import { expect } from 'chai';
import * as path from 'path';
import * as fs from 'fs';
import { BrowserWindow } from 'electron/main';
import { ifdescribe, ifit } from './spec-helpers';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';
import * as childProcess from 'child_process';

const Module = require('module');

const features = process._linkedBinding('electron_common_features');
const nativeModulesEnabled = !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS;

describe('modules support', () => {
  const fixtures = path.join(__dirname, 'fixtures');

  describe('third-party module', () => {
    ifdescribe(nativeModulesEnabled)('echo', () => {
      afterEach(closeAllWindows);
      it('can be required in renderer', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        w.loadURL('about:blank');
        await expect(w.webContents.executeJavaScript('{ require(\'echo\'); null }')).to.be.fulfilled();
      });

      ifit(features.isRunAsNodeEnabled())('can be required in node binary', async function () {
        const child = childProcess.fork(path.join(fixtures, 'module', 'echo.js'));
        const [msg] = await emittedOnce(child, 'message');
        expect(msg).to.equal('ok');
      });

      ifit(process.platform === 'win32')('can be required if electron.exe is renamed', () => {
        const testExecPath = path.join(path.dirname(process.execPath), 'test.exe');
        fs.copyFileSync(process.execPath, testExecPath);
        try {
          const fixture = path.join(fixtures, 'module', 'echo-renamed.js');
          expect(fs.existsSync(fixture)).to.be.true();
          const child = childProcess.spawnSync(testExecPath, [fixture]);
          expect(child.status).to.equal(0);
        } finally {
          fs.unlinkSync(testExecPath);
        }
      });
    });

    const enablePlatforms: NodeJS.Platform[] = [
      'linux',
      'darwin',
      'win32'
    ];
    ifdescribe(nativeModulesEnabled && enablePlatforms.includes(process.platform))('module that use uv_dlopen', () => {
      it('can be required in renderer', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        w.loadURL('about:blank');
        await expect(w.webContents.executeJavaScript('{ require(\'uv-dlopen\'); null }')).to.be.fulfilled();
      });

      ifit(features.isRunAsNodeEnabled())('can be required in node binary', async function () {
        const child = childProcess.fork(path.join(fixtures, 'module', 'uv-dlopen.js'));
        await new Promise<void>(resolve => child.once('exit', (exitCode) => {
          expect(exitCode).to.equal(0);
          resolve();
        }));
      });
    });

    describe('q', () => {
      describe('Q.when', () => {
        it('emits the fullfil callback', (done) => {
          const Q = require('q');
          Q(true).then((val: boolean) => {
            expect(val).to.be.true();
            done();
          });
        });
      });
    });

    describe('coffeescript', () => {
      it('can be registered and used to require .coffee files', () => {
        expect(() => {
          require('coffeescript').register();
        }).to.not.throw();
        expect(require('./fixtures/module/test.coffee')).to.be.true();
      });
    });
  });

  describe('global variables', () => {
    describe('process', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-process')).to.equal('declared process');
      });
    });

    describe('global', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-global')).to.equal('declared global');
      });
    });

    describe('Buffer', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-buffer')).to.equal('declared Buffer');
      });
    });
  });

  describe('Module._nodeModulePaths', () => {
    // Work around the hack in spec/global-paths.
    beforeEach(() => {
      Module.ignoreGlobalPathsHack = true;
    });

    afterEach(() => {
      Module.ignoreGlobalPathsHack = false;
    });

    describe('when the path is inside the resources path', () => {
      it('does not include paths outside of the resources path', () => {
        let modulePath = process.resourcesPath;
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = process.resourcesPath + '-foo';
        const nodeModulePaths = Module._nodeModulePaths(modulePath);
        expect(nodeModulePaths).to.include(path.join(modulePath, 'node_modules'));
        expect(nodeModulePaths).to.include(path.join(modulePath, '..', 'node_modules'));

        modulePath = path.join(process.resourcesPath, 'foo');
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo');
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'bar');
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar');
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);
      });
    });

    describe('when the path is outside the resources path', () => {
      it('includes paths outside of the resources path', () => {
        const modulePath = path.resolve('/foo');
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(modulePath, 'node_modules'),
          path.resolve('/node_modules')
        ]);
      });
    });
  });

  describe('require', () => {
    describe('when loaded URL is not file: protocol', () => {
      afterEach(closeAllWindows);
      it('searches for module under app directory', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        w.loadURL('about:blank');
        const result = await w.webContents.executeJavaScript('typeof require("q").when');
        expect(result).to.equal('function');
      });
    });
  });
});

import { BrowserWindow, utilityProcess } from 'electron/main';

import { afterEach, describe, expect, it } from 'vitest';

import * as childProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import { createRequire } from 'node:module';
import * as path from 'node:path';

import { ifdescribe, ifit, spawnAndWait } from './lib/spec-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

const require = createRequire(import.meta.url);

const Module = require('node:module') as NodeJS.ModuleInternal;

const nativeModulesEnabled = !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS;

describe('modules support', () => {
  const fixtures = path.join(import.meta.dirname, 'fixtures');

  describe('third-party module', () => {
    ifdescribe(nativeModulesEnabled)('echo', () => {
      afterEach(closeAllWindows);
      it('can be required in renderer', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('@electron-ci/echo'); null }");
      });

      it('can be required in node binary', async () => {
        const child = childProcess.fork(path.join(fixtures, 'module', 'echo.js'));
        const [msg] = await once(child, 'message');
        expect(msg).toBe('ok');
      });

      ifit(process.platform === 'win32')('can be required if electron.exe is renamed', async () => {
        const testExecPath = path.join(path.dirname(process.execPath), 'test.exe');
        fs.copyFileSync(process.execPath, testExecPath);
        try {
          const fixture = path.join(fixtures, 'module', 'echo-renamed.js');
          expect(fs.existsSync(fixture)).toBe(true);
          const { code, stdout, stderr } = await spawnAndWait(testExecPath, [fixture], { timeout: 20_000 });
          expect(code, `stdout:\n${stdout}\nstderr:\n${stderr}`).toBe(0);
        } finally {
          fs.unlinkSync(testExecPath);
        }
      });
    });

    const enablePlatforms: NodeJS.Platform[] = ['linux', 'darwin', 'win32'];
    ifdescribe(nativeModulesEnabled && enablePlatforms.includes(process.platform))('module that use uv_dlopen', () => {
      it('can be required in renderer', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('@electron-ci/uv-dlopen'); null }");
      });

      it('can be required in node binary', async () => {
        const child = childProcess.fork(path.join(fixtures, 'module', 'uv-dlopen.js'));
        const [exitCode] = await once(child, 'exit');
        expect(exitCode).toBe(0);
      });
    });

    describe('q', () => {
      describe('Q.when', () => {
        it('emits the fulfil callback', () =>
          new Promise<void>((resolve) => {
            const Q = require('q');
            Q(true).then((val: boolean) => {
              expect(val).toBe(true);
              resolve();
            });
          }));
      });
    });

    describe("require('electron/...')", () => {
      const utilityProcessFixturesPath = path.resolve(
        import.meta.dirname,
        'fixtures',
        'api',
        'utility-process',
        'electron-modules'
      );

      it("require('electron/lol') should throw in the main process", () => {
        expect(() => {
          require('electron/lol');
        }).toThrow(/Cannot find module 'electron\/lol'/);
      });

      it("require('electron/lol') should throw in the renderer process", async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await expect(w.webContents.executeJavaScript("{ require('electron/lol'); null }")).rejects.toThrow();
      });

      it("require('electron/lol') should throw in the utility process", async () => {
        const child = utilityProcess.fork(path.join(utilityProcessFixturesPath, 'require-lol.js'), [], {
          stdio: ['ignore', 'ignore', 'pipe']
        });
        const sawError = new Promise<string>((resolve) => {
          let stderr = '';
          child.stderr!.on('data', (data) => {
            stderr += data.toString('utf8');
            if (/Cannot find module 'electron\/lol'/.test(stderr)) resolve(stderr);
          });
        });
        const [code] = await once(child, 'exit');
        expect(code).toBe(1);
        expect(await sawError).toMatch(/Cannot find module 'electron\/lol'/);
      });

      it("require('electron') should not throw in the main process", () => {
        expect(() => {
          require('electron');
        }).not.toThrow();
      });

      it("require('electron') should not throw in the renderer process", async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('electron'); null }");
      });

      it("require('electron/main') should not throw in the main process", () => {
        expect(() => {
          require('electron/main');
        }).not.toThrow();
      });

      it("require('electron/main') should not throw in the renderer process", async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('electron/main'); null }");
      });

      it("require('electron/main') should not throw in the utility process", async () => {
        const child = utilityProcess.fork(path.join(utilityProcessFixturesPath, 'require-main.js'));
        const [code] = await once(child, 'exit');
        expect(code).toBe(0);
      });

      it("require('electron/renderer') should not throw in the main process", () => {
        expect(() => {
          require('electron/renderer');
        }).not.toThrow();
      });

      it("require('electron/renderer') should not throw in the renderer process", async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('electron/renderer'); null }");
      });

      it("require('electron/renderer') should not throw in the utility process", async () => {
        const child = utilityProcess.fork(path.join(utilityProcessFixturesPath, 'require-renderer.js'));
        const [code] = await once(child, 'exit');
        expect(code).toBe(0);
      });

      it("require('electron/common') should not throw in the main process", () => {
        expect(() => {
          require('electron/common');
        }).not.toThrow();
      });

      it("require('electron/common') should not throw in the renderer process", async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('electron/common'); null }");
      });

      it("require('electron/common') should not throw in the utility process", async () => {
        const child = utilityProcess.fork(path.join(utilityProcessFixturesPath, 'require-common.js'));
        const [code] = await once(child, 'exit');
        expect(code).toBe(0);
      });

      it("require('electron/utility') should not throw in the main process", () => {
        expect(() => {
          require('electron/utility');
        }).not.toThrow();
      });

      it("require('electron/utility') should not throw in the renderer process", async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        await w.webContents.executeJavaScript("{ require('electron/utility'); null }");
      });

      it("require('electron/utility') should not throw in the utility process", async () => {
        const child = utilityProcess.fork(path.join(utilityProcessFixturesPath, 'require-utility.js'));
        const [code] = await once(child, 'exit');
        expect(code).toBe(0);
      });
    });

    describe('coffeescript', () => {
      it('can be registered and used to require .coffee files', () => {
        expect(() => {
          require('coffeescript').register();
        }).not.toThrow();
        expect(require('./fixtures/module/test.coffee')).toBe(true);
      });
    });
  });

  describe('global variables', () => {
    describe('process', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-process')).toBe('declared process');
      });
    });

    describe('global', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-global')).toBe('declared global');
      });
    });

    describe('Buffer', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-buffer')).toBe('declared Buffer');
      });
    });
  });

  describe('Module._nodeModulePaths', () => {
    describe('when the path is inside the resources path', () => {
      it('does not include paths outside of the resources path', () => {
        let modulePath = process.resourcesPath;
        expect(Module._nodeModulePaths(modulePath)).toEqual([path.join(process.resourcesPath, 'node_modules')]);

        modulePath = process.resourcesPath + '-foo';
        const nodeModulePaths = Module._nodeModulePaths(modulePath);
        expect(nodeModulePaths).toContain(path.join(modulePath, 'node_modules'));
        expect(nodeModulePaths).toContain(path.join(modulePath, '..', 'node_modules'));

        modulePath = path.join(process.resourcesPath, 'foo');
        expect(Module._nodeModulePaths(modulePath)).toEqual([
          path.join(process.resourcesPath, 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo');
        expect(Module._nodeModulePaths(modulePath)).toEqual([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'bar');
        expect(Module._nodeModulePaths(modulePath)).toEqual([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar');
        expect(Module._nodeModulePaths(modulePath)).toEqual([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ]);
      });
    });

    describe('when the path is outside the resources path', () => {
      it('includes paths outside of the resources path', () => {
        const modulePath = path.resolve('/foo');
        expect(Module._nodeModulePaths(modulePath)).toEqual([
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
        const w = new BrowserWindow({
          show: false,
          webPreferences: { nodeIntegration: true, contextIsolation: false }
        });
        w.loadURL('about:blank');
        const result = await w.webContents.executeJavaScript('typeof require("q").when');
        expect(result).toBe('function');
      });
    });
  });

  describe('esm', () => {
    it('can load the built-in "electron" module via ESM import', async () => {
      await expect(import('electron')).resolves.toBeTruthy();
    });

    it('the built-in "electron" module loaded via ESM import has the same exports as the CJS module', async () => {
      const esmElectron = await import('electron');
      const cjsElectron = require('electron');
      expect(Object.keys(esmElectron)).toEqual(expect.arrayContaining(Object.keys(cjsElectron)));
    });
  });
});

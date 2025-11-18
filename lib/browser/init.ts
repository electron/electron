import type * as defaultMenuModule from '@electron/internal/browser/default-menu';

import { EventEmitter } from 'events';
import * as fs from 'fs';
import * as path from 'path';

import type * as url from 'url';
import type * as v8 from 'v8';

const Module = require('module') as NodeJS.ModuleInternal;

process.argv.splice(1, 1);

require('@electron/internal/common/init');

process._linkedBinding('electron_browser_event_emitter').setEventEmitterPrototype(EventEmitter.prototype);

process.on('uncaughtException', function (error) {
  // Do nothing if the user has a custom uncaught exception handler.
  if (process.listenerCount('uncaughtException') > 1) {
    return;
  }

  import('electron')
    .then(({ dialog }) => {
      const stack = error.stack ? error.stack : `${error.name}: ${error.message}`;
      const message = 'Uncaught Exception:\n' + stack;
      dialog.showErrorBox('A JavaScript error occurred in the main process', message);
    });
});

const { app } = require('electron');

app.on('quit', (_event: any, exitCode: number) => {
  process.emit('exit', exitCode);
});

if (process.platform === 'win32') {
  // Set app user model ID for Squirrel.Windows installations
  const updateDotExe = path.join(path.dirname(process.execPath), '..', 'update.exe');

  if (fs.existsSync(updateDotExe)) {
    const packageDir = path.dirname(path.resolve(updateDotExe));
    const packageName = path.basename(packageDir).replaceAll(/\s/g, '');
    const exeName = path.basename(process.execPath).replace(/\.exe$/i, '').replaceAll(/\s/g, '');

    app.setAppUserModelId(`com.squirrel.${packageName}.${exeName}`);
  }
}

process.exit = app.exit as () => never;

require('@electron/internal/browser/rpc-server');
require('@electron/internal/browser/guest-view-manager');

const v8Util = process._linkedBinding('electron_common_v8_util');
let packagePath = null;
let packageJson = null;
const searchPaths: string[] = v8Util.getHiddenValue(global, 'appSearchPaths');
const searchPathsOnlyLoadASAR: boolean = v8Util.getHiddenValue(global, 'appSearchPathsOnlyLoadASAR');
const getOrCreateArchive = process._getOrCreateArchive;
delete process._getOrCreateArchive;

if (process.resourcesPath) {
  for (packagePath of searchPaths) {
    try {
      packagePath = path.join(process.resourcesPath, packagePath);
      if (searchPathsOnlyLoadASAR) {
        if (!getOrCreateArchive?.(packagePath)) {
          continue;
        }
      }
      packageJson = Module._load(path.join(packagePath, 'package.json'));
      break;
    } catch {
      continue;
    }
  }
}

if (packageJson == null) {
  process.nextTick(function () {
    return process.exit(1);
  });
  throw new Error('Unable to find a valid app');
}

if (packageJson.version != null) {
  app.setVersion(packageJson.version);
}

if (packageJson.productName != null) {
  app.name = `${packageJson.productName}`.trim();
} else if (packageJson.name != null) {
  app.name = `${packageJson.name}`.trim();
}

if (packageJson.desktopName != null) {
  app.setDesktopName(packageJson.desktopName);
} else {
  app.setDesktopName(`${app.name}.desktop`);
}

// Lazy load v8 module
if (packageJson.v8Flags != null) {
  (require('v8') as typeof v8).setFlagsFromString(packageJson.v8Flags);
}

app.setAppPath(packagePath);

require('@electron/internal/browser/devtools');
require('@electron/internal/browser/api/protocol');
require('@electron/internal/browser/api/service-worker-main');
require('@electron/internal/browser/api/web-contents');
require('@electron/internal/browser/api/web-frame-main');
require('@electron/internal/browser/api/web-contents-view');

const mainStartupScript = packageJson.main || 'index.js';

app.on('window-all-closed', () => {
  if (app.listenerCount('window-all-closed') === 1) {
    app.quit();
  }
});

const { setDefaultApplicationMenu } = require('@electron/internal/browser/default-menu') as typeof defaultMenuModule;

app.once('will-finish-launching', setDefaultApplicationMenu);

const { appCodeLoaded } = process;
delete process.appCodeLoaded;

if (packagePath) {
  if ((packageJson.type === 'module' && !mainStartupScript.endsWith('.cjs')) || mainStartupScript.endsWith('.mjs')) {
    const { runEntryPointWithESMLoader } = __non_webpack_require__('internal/modules/run_main') as typeof import('@node/lib/internal/modules/run_main');
    const main = (require('url') as typeof url).pathToFileURL(path.join(packagePath, mainStartupScript));
    runEntryPointWithESMLoader(async (cascadedLoader: any) => {
      try {
        await cascadedLoader.import(main.toString(), undefined, Object.create(null));
        appCodeLoaded!();
      } catch (err) {
        appCodeLoaded!();
        process.emit('uncaughtException', err as Error);
      }
    });
  } else {
    appCodeLoaded!();
    Module._load(path.join(packagePath, mainStartupScript), Module, true);
  }
} else {
  console.error('Failed to locate a valid package to load (app, app.asar or default_app.asar)');
  console.error('This normally means you\'ve damaged the Electron package somehow');
  appCodeLoaded!();
}

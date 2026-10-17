import { EventEmitter } from 'events';
import * as path from 'path';

import type * as url from 'url';
import type * as v8 from 'v8';

const Module = require('module') as NodeJS.ModuleInternal;

// Import common settings.
require('@electron/internal/common/init');

process._linkedBinding('electron_browser_event_emitter').setEventEmitterPrototype(EventEmitter.prototype);

// Don't quit on fatal error.
process.on('uncaughtException', function (error) {
  // Do nothing if the user has a custom uncaught exception handler.
  if (process.listenerCount('uncaughtException') > 1) {
    return;
  }

  // Show error in GUI.
  // We can't import { dialog } at the top of this file as this file is
  // responsible for setting up the require hook for the "electron" module
  // so we import it inside the handler down here
  Promise.resolve().then(() => {
    const { dialog } = require('electron/main') as typeof import('electron/main');
    const stack = error.stack ? error.stack : `${error.name}: ${error.message}`;
    const message = 'Uncaught Exception:\n' + stack;
    dialog.showErrorBox('A JavaScript error occurred in the main process', message);
  });
});

// Emit 'exit' event on quit.
const { app } = require('electron');

app.on('quit', (_event: any, exitCode: number) => {
  process.emit('exit', exitCode);
});

// Map process.exit to app.exit, which quits gracefully. When called without
// an explicit code, fall back to process.exitCode like Node.js does.
process.exit = ((code: number | string | undefined | null) => {
  // Refs https://github.com/nodejs/node/blob/fc192ee030ee076b948ce7d9d72cba6c101989b8/lib/internal/process/per_thread.js#L229-L252
  if (code !== undefined) {
    // Node.js handles any string to number conversion here for us
    process.exitCode = code;
  }

  app.exit(process.exitCode || 0);
}) as typeof process.exit;

// Deliver IPC from renderers to ipcMain and friends.
require('@electron/internal/browser/ipc-dispatch');

// Load the RPC server.
require('@electron/internal/browser/rpc-server');

// Load the guest view manager.
require('@electron/internal/browser/guest-view-manager');

// The app's package.json has already been found and applied (name, version,
// desktopName); what is left is the entry script and any v8Flags for it.
const appPackage = process
  ._linkedBinding('electron_common_v8_util')
  .getHiddenValue<{ path: string; main: string; esm: boolean; v8Flags?: string }>(global, 'appPackage');

if (!appPackage) {
  process.nextTick(function () {
    return process.exit(1);
  });
  throw new Error('Unable to find a valid app');
}

app.setAppPath(appPackage.path);

// Load protocol module to ensure it is populated on app ready
require('@electron/internal/browser/api/protocol');

// Load service-worker-main module to ensure it is populated on app ready
require('@electron/internal/browser/api/service-worker-main');

// Load web-contents module to ensure it is populated on app ready
require('@electron/internal/browser/api/web-contents');

// Load web-frame-main module to ensure it is populated on app ready
require('@electron/internal/browser/api/web-frame-main');

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', () => {
  if (app.listenerCount('window-all-closed') === 1) {
    app.quit();
  }
});

const { appCodeLoaded } = process;
delete process.appCodeLoaded;

// Applied only now so that everything above still matched its code cache.
if (appPackage.v8Flags) {
  (require('v8') as typeof v8).setFlagsFromString(appPackage.v8Flags);
}

// Finally load app's main script and transfer control to C++.
if (appPackage.esm) {
  const { runEntryPointWithESMLoader } =
    require('internal/modules/run_main') as typeof import('@node/lib/internal/modules/run_main');
  const main = (require('url') as typeof url).pathToFileURL(path.join(appPackage.path, appPackage.main));
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
  // Call appCodeLoaded before just for safety, it doesn't matter here as _load is synchronous
  appCodeLoaded!();
  Module._load(path.join(appPackage.path, appPackage.main), Module, true);
}

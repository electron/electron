import * as fs from 'fs';
import * as path from 'path';

import { deprecate, Menu } from 'electron/main';
import { EventEmitter } from 'events';

const bindings = process._linkedBinding('electron_browser_app');
const commandLine = process._linkedBinding('electron_common_command_line');
const { app, App } = bindings;

// Only one app object permitted.
export default app;

let dockMenu: Electron.Menu | null = null;

// App is an EventEmitter.
Object.setPrototypeOf(App.prototype, EventEmitter.prototype);
EventEmitter.call(app as any);

// Properties.

const nativeASGetter = app.isAccessibilitySupportEnabled;
const nativeASSetter = app.setAccessibilitySupportEnabled;
Object.defineProperty(App.prototype, 'accessibilitySupportEnabled', {
  get: () => nativeASGetter.call(app),
  set: (enabled) => nativeASSetter.call(app, enabled)
});

const nativeBCGetter = app.getBadgeCount;
const nativeBCSetter = app.setBadgeCount;
Object.defineProperty(App.prototype, 'badgeCount', {
  get: () => nativeBCGetter.call(app),
  set: (count) => nativeBCSetter.call(app, count)
});

const nativeNGetter = app.getName;
const nativeNSetter = app.setName;
Object.defineProperty(App.prototype, 'name', {
  get: () => nativeNGetter.call(app),
  set: (name) => nativeNSetter.call(app, name)
});

Object.assign(app, {
  commandLine: {
    hasSwitch: (theSwitch: string) => commandLine.hasSwitch(String(theSwitch)),
    getSwitchValue: (theSwitch: string) => commandLine.getSwitchValue(String(theSwitch)),
    appendSwitch: (theSwitch: string, value?: string) => commandLine.appendSwitch(String(theSwitch), typeof value === 'undefined' ? value : String(value)),
    appendArgument: (arg: string) => commandLine.appendArgument(String(arg))
  } as Electron.CommandLine
});

// we define this here because it'd be overly complicated to
// do in native land
Object.defineProperty(app, 'applicationMenu', {
  get () {
    return Menu.getApplicationMenu();
  },
  set (menu: Electron.Menu | null) {
    return Menu.setApplicationMenu(menu);
  }
});

App.prototype.isPackaged = (() => {
  const execFile = path.basename(process.execPath).toLowerCase();
  if (process.platform === 'win32') {
    return execFile !== 'electron.exe';
  }
  return execFile !== 'electron';
})();

app._setDefaultAppPaths = (packagePath) => {
  // Set the user path according to application's name.
  app.setPath('userData', path.join(app.getPath('appData'), app.name!));
  app.setPath('userCache', path.join(app.getPath('cache'), app.name!));
  app.setAppPath(packagePath);

  // Add support for --user-data-dir=
  if (app.commandLine.hasSwitch('user-data-dir')) {
    const userDataDir = app.commandLine.getSwitchValue('user-data-dir');
    if (path.isAbsolute(userDataDir)) app.setPath('userData', userDataDir);
  }
};

if (process.platform === 'darwin') {
  const setDockMenu = app.dock!.setMenu;
  app.dock!.setMenu = (menu) => {
    dockMenu = menu;
    setDockMenu(menu);
  };
  app.dock!.getMenu = () => dockMenu;
}

if (process.platform === 'linux') {
  const patternVmRSS = /^VmRSS:\s*(\d+) kB$/m;
  const patternVmHWM = /^VmHWM:\s*(\d+) kB$/m;

  const getStatus = (pid: number) => {
    try {
      return fs.readFileSync(`/proc/${pid}/status`, 'utf8');
    } catch {
      return '';
    }
  };

  const getEntry = (file: string, pattern: RegExp) => {
    const match = file.match(pattern);
    return match ? parseInt(match[1], 10) : 0;
  };

  const getProcessMemoryInfo = (pid: number) => {
    const file = getStatus(pid);

    return {
      workingSetSize: getEntry(file, patternVmRSS),
      peakWorkingSetSize: getEntry(file, patternVmHWM)
    };
  };

  const nativeFn = app.getAppMetrics;
  app.getAppMetrics = () => {
    const metrics = nativeFn.call(app);
    for (const metric of metrics) {
      metric.memory = getProcessMemoryInfo(metric.pid);
    }

    return metrics;
  };
}

// Routes the events to webContents.
const events = ['certificate-error', 'select-client-certificate'];
for (const name of events) {
  app.on(name as 'certificate-error', (event, webContents, ...args: any[]) => {
    webContents.emit(name, event, ...args);
  });
}

// Deprecate allowRendererProcessReuse but only if they set it to false, no need to log if
// they are setting it to true
deprecate.removeProperty(app, 'allowRendererProcessReuse', [false]);

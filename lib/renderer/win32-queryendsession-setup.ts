import { ipcRenderer } from 'electron';

export function setup (binding: typeof process['_linkedBinding']) {
  // TODO(codebytere): fix typedef here.
  (binding as any).setShutdownHandler(() => {
    ipcRenderer.send('ELECTRON_BROWSER_QUERYENDSESSION');
  });
}

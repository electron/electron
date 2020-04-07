import { Event } from '@electron/internal/renderer/extensions/event';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

class WebNavigation {
  private onBeforeNavigate = new Event()
  private onCompleted = new Event()

  constructor () {
    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', (event: Electron.IpcRendererEvent, details: any) => {
      this.onBeforeNavigate.emit(details);
    });

    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONCOMPLETED', (event: Electron.IpcRendererEvent, details: any) => {
      this.onCompleted.emit(details);
    });
  }
}

export const setup = () => new WebNavigation();

import { Event } from '@electron/internal/renderer/extensions/event';
import { IpcMainEvent } from 'electron';
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal');

class WebNavigation {
  private onBeforeNavigate = new Event()
  private onCompleted = new Event()

  constructor () {
    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', (event: IpcMainEvent, details: any) => {
      this.onBeforeNavigate.emit(details);
    });

    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONCOMPLETED', (event: IpcMainEvent, details: any) => {
      this.onCompleted.emit(details);
    });
  }
}

export const setup = () => new WebNavigation();

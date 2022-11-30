import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';

export const ipcMainInternal = new IpcMainImpl() as ElectronInternal.IpcMainInternal;
